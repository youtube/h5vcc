#!/usr/bin/perl -w
#
# Copyright (C) 2011 Google Inc.  All rights reserved.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public License
# along with this library; see the file COPYING.LIB.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.
#

use strict;

use File::Basename;
use Getopt::Long;
use Cwd;

use IDLParser;

my $defines;
my $preprocessor;
my $verbose;
my $idlFilesList;
my $idlAttributesFile;
my $namespaceBindingsPath;
my $supplementalDependencyFile;
my $supplementalMakefileDeps;

GetOptions('defines=s' => \$defines,
           'preprocessor=s' => \$preprocessor,
           'verbose' => \$verbose,
           'idlFilesList=s' => \$idlFilesList,
           'idlAttributesFile=s' => \$idlAttributesFile,
           'namespaceBindingsPath=s' => \$namespaceBindingsPath,
           'supplementalDependencyFile=s' => \$supplementalDependencyFile,
           'supplementalMakefileDeps=s' => \$supplementalMakefileDeps);

die('Must specify #define macros using --defines.') unless defined($defines);
die('Must specify an output file using --supplementalDependencyFile.') unless defined($supplementalDependencyFile);
die('Must specify the file listing all IDLs using --idlFilesList.') unless defined($idlFilesList);
die('Must specify a path to generate namespace bindings using --namespaceBindingsPath.') unless ($namespaceBindingsPath);


if ($verbose) {
    print "Resolving [Supplemental=XXX] and [Namespace=XXX] dependencies in all IDL files.\n";
}

open FH, "< $idlFilesList" or die "Cannot open $idlFilesList\n";
my @idlFiles = <FH>;
chomp(@idlFiles);
close FH;

# Parse all IDL files.
my %documents;
my %interfaceNameToIdlFile;
my %idlFileToInterfaceName;
foreach my $idlFile (@idlFiles) {
    my $fullPath = Cwd::realpath($idlFile);
    my $parser = IDLParser->new(!$verbose);
    $documents{$fullPath} = $parser->Parse($idlFile, $defines, $preprocessor);
    my $interfaceName = fileparse(basename($idlFile), ".idl");
    $interfaceNameToIdlFile{$interfaceName} = $fullPath;
    $idlFileToInterfaceName{$fullPath} = $interfaceName;
}

# Runs the IDL attribute checker.
my $idlAttributes = loadIDLAttributes($idlAttributesFile);
foreach my $idlFile (keys %documents) {
    checkIDLAttributes($idlAttributes, $documents{$idlFile}, basename($idlFile));
}

# Resolves [Supplemental=XXX] and [Namespace=XXX] dependencies.
my %supplementals;
my %namespaces;
foreach my $idlFile (keys %documents) {
    $supplementals{$idlFile} = [];
}
foreach my $idlFile (keys %documents) {
    foreach my $interface (@{$documents{$idlFile}->interfaces}) {
        if ($interface->extendedAttributes->{"Namespace"}) {
            if ($interface->extendedAttributes->{"Supplemental"}) {
                die "IDL ATTRIBUTE ERROR: [Namespace] and [Supplemental] are mutually exclusive, found at $idlFile\n";
            }

            # Break the [Namespace] string into an array.
            # The format is namespace components separated by :: instead of dots.
            # The lexer won't accept dots.
            my @ns = split(/::/, $interface->extendedAttributes->{"Namespace"});
            die "Empty namespace not acceptable, use Supplemental=DOMWindow instead.\n" if (!scalar(@ns));

            # Add the final object name to the @ns array.
            my $name = $idlFileToInterfaceName{$idlFile};

            my $ns_top = $ns[0];
            injectNamespace($ns_top);

            # Add the namespace and conditional info to a hash.
            if (!$namespaces{$ns_top}) {
              $namespaces{$ns_top} = [];
            }
            my $ns_info = {};
            $ns_info->{"ns_ref"} = \@ns;
            $ns_info->{"final_component"} = $name;
            $ns_info->{"interface"} = $interface;
            push(@{$namespaces{$ns_top}}, $ns_info);
        } elsif ($interface->extendedAttributes->{"Supplemental"}) {
            my $targetIdlFile = $interfaceNameToIdlFile{$interface->extendedAttributes->{"Supplemental"}};
            push(@{$supplementals{$targetIdlFile}}, $idlFile);
            # Treats as if this IDL file does not exist.
            delete $supplementals{$idlFile};
        }
    }
}
foreach my $ns_top (keys %namespaces) {
    my $ns_list_ref = $namespaces{$ns_top};
    generateNamespaceCpp($ns_top, $ns_list_ref);
}

# Outputs the dependency.
# The format of a supplemental dependency file:
#
# DOMWindow.idl P.idl Q.idl R.idl
# Document.idl S.idl
# Event.idl
# ...
#
# The above indicates that DOMWindow.idl is supplemented by P.idl, Q.idl and R.idl,
# Document.idl is supplemented by S.idl, and Event.idl is supplemented by no IDLs.
# The IDL that supplements another IDL (e.g. P.idl) never appears in the dependency file.

open FH, "> $supplementalDependencyFile" or die "Cannot open $supplementalDependencyFile\n";

foreach my $idlFile (sort keys %supplementals) {
    print FH $idlFile, " @{$supplementals{$idlFile}}\n";
}
close FH;


if ($supplementalMakefileDeps) {
    open MAKE_FH, "> $supplementalMakefileDeps" or die "Cannot open $supplementalMakefileDeps\n";
    my @all_dependencies = [];
    foreach my $idlFile (sort keys %supplementals) {
        my $basename = $idlFileToInterfaceName{$idlFile};

        my @dependencies = map { basename($_) } @{$supplementals{$idlFile}};

        print MAKE_FH "V8${basename}.h: @{dependencies}\n";
        print MAKE_FH "DOM${basename}.h: @{dependencies}\n";
        print MAKE_FH "WebDOM${basename}.h: @{dependencies}\n";
        foreach my $dependency (@dependencies) {
            print MAKE_FH "${dependency}:\n";
        }
    }

    close MAKE_FH;
}


sub loadIDLAttributes
{
    my $idlAttributesFile = shift;

    my %idlAttributes;
    open FH, "<", $idlAttributesFile or die "Couldn't open $idlAttributesFile: $!";
    while (my $line = <FH>) {
        chomp $line;
        next if $line =~ /^\s*#/;
        next if $line =~ /^\s*$/;

        if ($line =~ /^\s*([^=\s]*)\s*=?\s*(.*)/) {
            my $name = $1;
            $idlAttributes{$name} = {};
            if ($2) {
                foreach my $rightValue (split /\|/, $2) {
                    $rightValue =~ s/^\s*|\s*$//g;
                    $rightValue = "VALUE_IS_MISSING" unless $rightValue;
                    $idlAttributes{$name}{$rightValue} = 1;
                }
            } else {
                $idlAttributes{$name}{"VALUE_IS_MISSING"} = 1;
            }
        } else {
            die "The format of " . basename($idlAttributesFile) . " is wrong: line $.\n";
        }
    }
    close FH;

    return \%idlAttributes;
}

sub checkIDLAttributes
{
    my $idlAttributes = shift;
    my $document = shift;
    my $idlFile = shift;

    foreach my $interface (@{$document->interfaces}) {
        checkIfIDLAttributesExists($idlAttributes, $interface->extendedAttributes, $idlFile);

        foreach my $attribute (@{$interface->attributes}) {
            checkIfIDLAttributesExists($idlAttributes, $attribute->signature->extendedAttributes, $idlFile);
        }

        foreach my $function (@{$interface->functions}) {
            checkIfIDLAttributesExists($idlAttributes, $function->signature->extendedAttributes, $idlFile);
            foreach my $parameter (@{$function->parameters}) {
                checkIfIDLAttributesExists($idlAttributes, $parameter->extendedAttributes, $idlFile);
            }
        }
    }
}

sub checkIfIDLAttributesExists
{
    my $idlAttributes = shift;
    my $extendedAttributes = shift;
    my $idlFile = shift;

    my $error;
    OUTER: for my $name (keys %$extendedAttributes) {
        if (!exists $idlAttributes->{$name}) {
            $error = "Unknown IDL attribute [$name] is found at $idlFile.";
            last OUTER;
        }
        if ($idlAttributes->{$name}{"*"}) {
            next;
        }
        for my $rightValue (split /\s*\|\s*/, $extendedAttributes->{$name}) {
            if (!exists $idlAttributes->{$name}{$rightValue}) {
                $error = "Unknown IDL attribute [$name=" . $extendedAttributes->{$name} . "] is found at $idlFile.";
                last OUTER;
            }
        }
    }
    if ($error) {
        die "IDL ATTRIBUTE CHECKER ERROR: $error
If you want to add a new IDL attribute, you need to add it to WebCore/bindings/scripts/IDLAttributes.txt and add explanations to the WebKit IDL document (https://trac.webkit.org/wiki/WebKitIDL).
";
    }
}

sub injectNamespace
{
    # Create an injector to add this namespace hierarchy to the parent.
    my $ns_top = shift;
    my $parent = "DOMWindow";
    my $injector = $parent . "__inject__" . $ns_top;
    my $type = "Object";

    # Create an injector interface to attach this namespace to the parent.
    my $filename = "$namespaceBindingsPath/$injector.idl";
    mkdir($namespaceBindingsPath);
    open FH, "> $filename" or die "Cannot open $filename for writing!\n";
    print FH "[\n";
    print FH "    Supplemental=$parent\n";
    print FH "] interface $injector {\n";
    print FH "    [CustomGetter=NamespaceInternal::$ns_top] readonly attribute $type $ns_top;\n";
    print FH "};\n";
    close FH;
    $interfaceNameToIdlFile{$injector} = $filename;

    # Without this check, you could end up recording a namespace dep twice,
    # which breaks the build.
    if (!exists($supplementals{$filename})) {
        $supplementals{$filename} = [];
        # Record the dependency between these two interfaces.
        my $parentIdlFile = $interfaceNameToIdlFile{$parent};
        die "Parent interface $parent must exist by now!\n" if (!$parentIdlFile);
        die "Supplementals not available for $parentIdlFile!\n" if (!$supplementals{$parentIdlFile});
        push(@{$supplementals{$parentIdlFile}}, $filename);
    }
}

sub generateNamespaceCpp
{
    my $ns_top = shift;
    my $ns_list_ref = shift;
    my @ns_list = @{$ns_list_ref};

    my $cppPath = "$namespaceBindingsPath/bindings";
    mkdir($cppPath);
    my $filename = "$cppPath/V8DOMWindow__inject__${ns_top}.cpp";
    my $property = lc($ns_top);

    open FH, "> $filename" or die "Cannot open $filename for writing!\n";
    print FH "#include \"config.h\"\n";
    print FH "#include \"V8DOMWindow.h\"\n";

    foreach my $ns_info (@ns_list) {
        my $final_component = $ns_info->{"final_component"};
        print FH "#include \"V8${final_component}.h\"\n";
    }

    print FH "\n";
    print FH "namespace WebCore {\n";
    print FH "namespace V8NamespaceInternal {\n";
    print FH "\n";
    print FH "v8::Handle<v8::Value> ${property}AccessorGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info) {\n";
    print FH "static v8::Persistent<v8::Object> cached_${property};\n";
    print FH "if (cached_${property}.IsEmpty()) {\n";
    print FH "  cached_${property} = v8::Persistent<v8::Object>::New(v8::Object::New());\n";

    my %attached;
    foreach my $ns_info (@ns_list) {
        my @ns = @{$ns_info->{"ns_ref"}};
        my $top = $ns[0];
        die "Namespace does not match! ($top should be $ns_top)\n" if ($top ne $ns_top);

        generateNSAttachment(\*FH, \%attached, $property, $ns_info);
    }

    print FH "  }\n";
    print FH "\n";
    print FH "  return cached_$property;\n";
    print FH "\n";
    print FH "}\n";
    print FH "\n";
    print FH "}  // namespace V8NamespaceInternal\n";
    print FH "}  // namespace WebCore\n";
    close FH;

    $filename =~ s/\.cpp$/.h/;
    open FH, "> $filename" or die "Cannot open $filename for writing!\n";
    print FH "#ifndef V8DOMWindow__inject__${ns_top}_H\n";
    print FH "#define V8DOMWindow__inject__${ns_top}_H\n";
    print FH "\n";
    print FH "namespace WebCore {\n";
    print FH "namespace V8NamespaceInternal {\n";
    print FH "\n";
    print FH "v8::Handle<v8::Value> ${property}AccessorGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info);\n";
    print FH "\n";
    print FH "}  // namespace V8NamespaceInternal\n";
    print FH "}  // namespace WebCore\n";
    print FH "\n";
    print FH "#endif\n";
    close FH;
}

sub generateNSAttachment
{
    my $fh = shift;
    my $attached = shift;
    my $property = shift;
    my $ns_info = shift;

    my @ns = @{$ns_info->{"ns_ref"}};
    my $final_component = $ns_info->{"final_component"};
    my $interface = $ns_info->{"interface"};
    my $conditional = $interface->extendedAttributes->{"Conditional"};
    my $constructor = $interface->extendedAttributes->{"Constructor"};
    my @attributes = @{$interface->attributes};
    my @functions = @{$interface->functions};

    my $parent_name = "cached_$property";

    # Start at index 1, because cached_namespace takes care of index 0.
    for (my $i = 1; $i < scalar(@ns); $i++) {
        my $component = $ns[$i];
        my $ns_path = join(".", @ns[0..$i]);
        my $ns_path_name = "temp_" . join("_", @ns[0..$i]);

        if (!$attached->{$ns_path}) {
          print $fh "\n";
          print $fh "    // Attach $ns_path\n";
          print $fh "    v8::Handle<v8::Object> ${ns_path_name} = v8::Object::New();\n";
          print $fh "    $parent_name->Set(v8::String::NewSymbol(\"${component}\"), ${ns_path_name});\n";
          print $fh "#if ENABLE($conditional)\n" if ($conditional);
          print $fh "    v8::Local<v8::Function> ${ns_path_name}_template = V8${final_component}::GetTemplate()->GetFunction();\n";
          print $fh "    v8::Local<v8::Object> ${ns_path_name}_proto = V8${final_component}::GetTemplate()->PrototypeTemplate()->NewInstance();\n";
          print $fh "#endif\n" if ($conditional);

          $attached->{$ns_path} = 1;
        }

        $parent_name = $ns_path_name;
    }

    # Attach the final component.
    my $ns_path = join(".", @ns) . "." . $final_component;
    my $ns_path_name = "temp_" . join("_", @ns);

    print $fh "\n";
    print $fh "#if ENABLE($conditional)\n" if ($conditional);

    if ($constructor) {
        print $fh "    // Attach $ns_path\n";
        print $fh "    ${parent_name}\->Set(v8::String::NewSymbol(\"${final_component}\"), ${ns_path_name}_template);\n";
    } else {
        print $fh "    // Attach $ns_path methods\n";
        foreach my $func (@functions) {
            my $name = $func->signature->name;
            print $fh "    ${ns_path_name}\->Set(v8::String::NewSymbol(\"$name\"), ${ns_path_name}_template->Get(v8::String::New(\"$name\")));\n";
        }

        print $fh "\n";
        print $fh "    // Attach $ns_path attributes\n";
        foreach my $attr (@attributes) {
            my $name = $attr->signature->name;
            print $fh "    ${ns_path_name}\->Set(v8::String::NewSymbol(\"$name\"), ${ns_path_name}_proto->Get(v8::String::New(\"$name\")));\n";
        }
    }

    print $fh "#endif\n" if ($conditional);
}

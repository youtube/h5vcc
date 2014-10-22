"""Utilities for package_application scripts."""

import abc
import logging
import multiprocessing
import os
import shutil
import subprocess
import sys
import time

_DEFAULT_USER = 'youtube-steel-buildbot'
_SSH_PREFIX = '{user}@steel-build.corp.google.com'


def SpawnProcess(cmd_line, env=None, cwd=None, shell=None):
  """Spawn a process that executes the command line."""

  logging.debug('Executing %s', cmd_line)
  if cwd:
    logging.debug('cwd is %s', cwd)

  if shell is None:
    # By default if the user passes in a command as a string,
    # assume they want it run under a subshell.
    # For lists of commands, assume they don't.
    shell = not isinstance(cmd_line, list)

  return subprocess.Popen(
      cmd_line, shell=shell, stdout=subprocess.PIPE,
      stderr=subprocess.STDOUT, env=env, cwd=cwd)


def Execute(cmd_line, env=None, cwd=None, shell=None):
  """Run the specific command line and handle error."""

  proc = SpawnProcess(cmd_line, env, cwd, shell)
  for line in proc.stdout:
    sys.stdout.write(line)
  proc.wait()
  if proc.returncode != 0:
    msg = '%s failed (%d)' % (cmd_line, proc.returncode)
    raise RuntimeError(msg)
  return 0


def ExecuteMultiple(
    cmd_line_list,
    env=None,
    cwd=None,
    shell=None,
    timeout=None):
  """Spawn multiple processes for each argument in the list.

  Launch a process for each item in cmd_line_list. Return a list
  containing the return code and stdout for each one.

  Args:
    cmd_line_list: List of command lines to execute.
    env: Dictionary of any additional environment variable settings.
    cwd: Working directory to set for the processes.
    shell: Should we execute the commands in a subshell or not.
    timeout: Seconds to wait for all processes.  If None, wait forever.

  Returns:
    List of tuples containing returncode, stdout for each process.
  """

  # List of created command_line and Popen objects.
  process_info = {}
  results = []
  for i, cmd_line in enumerate(cmd_line_list):
    proc = SpawnProcess(cmd_line, env, cwd, shell)
    process_info[proc] = (i, cmd_line)
    results.append((0, ''))

  # Wait for all, and process the results.
  # Results are in the same order as the command line list.

  start_time = time.time()
  running_processes = process_info.keys()

  # Run until everything is done, or we timeout.
  while True:
    # Iterate over a copy, so we can remove elements from the list.
    for proc in running_processes[:]:
      if proc.poll() is not None:
        running_processes.remove(proc)
        proc_index, cmd_line = process_info[proc]
        output = '\n'.join(proc.stdout.readlines())
        results[proc_index] = proc.returncode, output

    # Handle timeouts, if set.
    if timeout is not None:
      if time.time() - start_time > timeout:
        # Abort any remaining processes if timeout deadline has been exceeded.
        for proc in running_processes:
          proc_index, cmd_line = process_info[proc]
          proc.kill()
          results[proc_index] = (-1, 'timed out')
        break

    # Continue to wait, or exit the loop.
    if running_processes:
      time.sleep(1)
    else:
      break

  return results


def WindowsPath(path):
  if sys.platform == 'cygwin':
    import cygpath  # pylint: disable=g-import-not-at-top
    return cygpath.to_nt(path)
  else:
    return path


def CopyFile(source, dest):
  logging.debug('Copying %s -> %s', source, dest)
  shutil.copy2(source, dest)


def MoveFile(source, dest):
  logging.debug('Moving %s -> %s', source, dest)
  os.rename(source, dest)


def IsBuildbot():
  return 'BUILDBOT_BUILDERNAME' in os.environ


class BasePackager(object):
  """Base class for all platform-specific Packager classes."""

  __metaclass__ = abc.ABCMeta

  def __init__(self, options):
    self.options = options

  @abc.abstractmethod
  def PackageApplication(self):
    """Run commands to package the application for this platform.

    This may involve zipping some files or uploading symbols to a symbol
    server, or running packaging tools from the platform SDK.
    Returns a list of the generated files that should be deployed
    to our staging server.
    """
    pass

  @staticmethod
  def AddCommandLineArguments(arg_parser):
    """Append any platform-specific command line arguments to the parser."""
    pass

  def SetupPaths(self, script_file, platform_name):
    """Set up some common paths."""
    self.platform_name = platform_name
    if not os.path.exists(script_file):
      raise RuntimeError('Invalid script file %s' % script_file)
    self._script_dir = os.path.abspath(os.path.dirname(script_file))
    self._root_dir = os.path.abspath(
        os.path.join(self._script_dir, '..', '..', '..'))
    self._out_dir = os.path.join(self._root_dir, 'out', 'lb_shell')

  def GetScriptDir(self):
    return self._script_dir

  def GetRootDir(self):
    return self._root_dir

  def GetOutDir(self):
    return self._out_dir

  def GetBuildDir(self, config):
    """Get the absolute build dir, e.g. ...out/lb_shell/Android_Devel."""
    return os.path.join(
        self._out_dir, '%s_%s' % (self.platform_name, config))

  def GetStagingDir(self, config):
    """Get the name of the staging directory.

    By convention we will place the staging files under the build dir.
    Platforms can override this if necessary.
    Args:
      config: Name of current build configuration (Debug, Devel, etc.)
    Raises:
      RuntimeError: if config has not been set.
    Returns:
      Path to the staging directory.
    """
    return os.path.join(self.GetBuildDir(config), 'staging')


def DeployToStaging(deployment_paths, staging_path, user=None):
  """Sync the given path to staging area on buildmaster.

  Only for use when running as a buildbot.
  Args:
    deployment_paths: List of paths to rsync.
    staging_path: Path to deploy to. Supplied by the buildbot.
                  E.g. android_builds/master/2014-06-07/
    user: User to log in to buildbot server as.  Only for testing.
  Returns:
    0 on success, non-0 on failure.
  """
  # Create the staging directory on the remote host.
  # Rsync can't do this itself.
  staging_path = 'staging/' + staging_path
  ssh_host_str = _SSH_PREFIX.format(user=user or _DEFAULT_USER)
  remote_mkdir = 'ssh %s "mkdir -p %s"' % (ssh_host_str, staging_path)
  try:
    Execute(remote_mkdir)
  except RuntimeError as e:
    logging.error(e)
    return 1

  if not isinstance(deployment_paths, list):
    deployment_paths = [deployment_paths]

  # Unique-ify deployment paths.
  deployment_paths = set(deployment_paths)
  logging.info('Transferring %s to %s', deployment_paths, staging_path)
  rsync_command = ['rsync', '-av']
  for local_path in deployment_paths:
    rsync_command.append(local_path)
  rsync_command.append('%s:%s' % (ssh_host_str, staging_path))
  try:
    Execute(rsync_command)
  except RuntimeError as e:
    logging.error(e)
    return 1
  logging.info('Deployment complete.')
  return 0


def Compress(job_list):
  """Take a list of (src, dest) pairs and spawn processes to compress them."""
  compress_procs = []
  outputs = []
  for source, dest in job_list:
    logging.info('Compressing %s -> %s.zip', source, dest)
    outputs.append('%s.zip' % dest)
    compress_procs.append(multiprocessing.Process(
        target=shutil.make_archive,
        args=(dest, 'zip', source, None)))

  for proc in compress_procs:
    proc.start()

  logging.debug('Waiting for %d compression jobs...', len(compress_procs))
  for proc in compress_procs:
    proc.join()
  return outputs

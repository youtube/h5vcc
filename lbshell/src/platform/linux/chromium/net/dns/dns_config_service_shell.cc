#include "net/dns/dns_config_service_shell.h"

#include "base/basictypes.h"

namespace net {

namespace internal {

DnsConfigServiceShell::DnsConfigServiceShell() {
  NOTIMPLEMENTED();
}

DnsConfigServiceShell::~DnsConfigServiceShell() {
  NOTIMPLEMENTED();
}

void DnsConfigServiceShell::ReadNow() {
  NOTIMPLEMENTED();
}

bool DnsConfigServiceShell::StartWatching() {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace internal

// static
scoped_ptr<DnsConfigService> DnsConfigService::CreateSystemService() {
  return scoped_ptr<DnsConfigService>(new internal::DnsConfigServiceShell());
}

}  // namespace net


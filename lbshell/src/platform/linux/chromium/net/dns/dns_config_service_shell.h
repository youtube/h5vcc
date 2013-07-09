#ifndef NET_DNS_DNS_CONFIG_SERVICE_SHELL_H_
#define NET_DNS_DNS_CONFIG_SERVICE_SHELL_H_

#include "net/dns/dns_config_service.h"

namespace net {

namespace internal {

class NET_EXPORT_PRIVATE DnsConfigServiceShell : public DnsConfigService {
 public:
  DnsConfigServiceShell();
  virtual ~DnsConfigServiceShell();

 private:
  // Immediately attempts to read the current configuration.
  virtual void ReadNow() OVERRIDE;

  // Registers system watchers. Returns true iff succeeds.
  virtual bool StartWatching() OVERRIDE;
};

}  // namespace internal

}  // namespace net

#endif  // NET_DNS_DNS_CONFIG_SERVICE_SHELL_H_

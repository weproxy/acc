//
// weproxy@foxmail.com 2022/10/03
//

#include <stdlib.h>
#include <pcap.h>
#include <stdint.h>
#include <poll.h>

// Some old versions of pcap don't define this constant.
#ifndef PCAP_NETMASK_UNKNOWN
#define PCAP_NETMASK_UNKNOWN 0xffffffff
#endif

// libpcap doesn't actually export its version in a #define-guardable way,
// so we have to use other defined things to differentiate versions.
// We assume at least libpcap v1.1 at the moment.
// See http://upstream-tracker.org/versions/libpcap.html

#ifndef PCAP_ERROR_TSTAMP_PRECISION_NOTSUP  // < v1.5
#define PCAP_ERROR_TSTAMP_PRECISION_NOTSUP -12

int pcap_set_immediate_mode(pcap_t *p, int mode) {
  return PCAP_ERROR;
}

//  libpcap version < v1.5 doesn't have timestamp precision (everything is microsecond)
//
//  This means *_tstamp_* functions and macros are missing. Therefore, we emulate these
//  functions here and pretend the setting the precision works. This is actually the way
//  the pcap_open_offline_with_tstamp_precision works, because it doesn't return an error
//  if it was not possible to set the precision, which depends on support by the given file.
//  => The rest of the functions always pretend as if they could set nano precision and
//  verify the actual precision with pcap_get_tstamp_precision, which is emulated for <v1.5
//  to always return micro resolution.

#define PCAP_TSTAMP_PRECISION_MICRO	0
#define PCAP_TSTAMP_PRECISION_NANO	1

pcap_t *pcap_open_offline_with_tstamp_precision(const char *fname, u_int precision,
  char *errbuf) {
  return pcap_open_offline(fname, errbuf);
}

pcap_t *pcap_fopen_offline_with_tstamp_precision(FILE *fp, u_int precision,
  char *errbuf) {
  return pcap_fopen_offline(fp, errbuf);
}

int pcap_set_tstamp_precision(pcap_t *p, int tstamp_precision) {
  if (tstamp_precision == PCAP_TSTAMP_PRECISION_MICRO)
    return 0;
  return PCAP_ERROR_TSTAMP_PRECISION_NOTSUP;
}

int pcap_get_tstamp_precision(pcap_t *p) {
  return PCAP_TSTAMP_PRECISION_MICRO;
}

#ifndef PCAP_TSTAMP_HOST  // < v1.2

int pcap_set_tstamp_type(pcap_t* p, int t) { return -1; }
int pcap_list_tstamp_types(pcap_t* p, int** t) { return 0; }
void pcap_free_tstamp_types(int *tstamp_types) {}
const char* pcap_tstamp_type_val_to_name(int t) {
	return "pcap timestamp types not supported";
}
int pcap_tstamp_type_name_to_val(const char* t) {
	return PCAP_ERROR;
}

#endif  // < v1.2
#endif  // < v1.5

#ifndef PCAP_ERROR_PROMISC_PERM_DENIED
#define PCAP_ERROR_PROMISC_PERM_DENIED -11
#endif

// Windows, Macs, and Linux all use different time types.  Joy.
#ifdef __APPLE__
#define gopacket_time_secs_t __darwin_time_t
#define gopacket_time_usecs_t __darwin_suseconds_t
#elif __ANDROID__
#define gopacket_time_secs_t __kernel_time_t
#define gopacket_time_usecs_t __kernel_suseconds_t
#elif __GLIBC__
#define gopacket_time_secs_t __time_t
#define gopacket_time_usecs_t __suseconds_t
#else  // Some form of linux/bsd/etc...
#include <sys/param.h>
#ifdef __OpenBSD__
#define gopacket_time_secs_t u_int32_t
#define gopacket_time_usecs_t u_int32_t
#else
#define gopacket_time_secs_t time_t
#define gopacket_time_usecs_t suseconds_t
#endif
#endif

// The things we do to avoid pointers escaping to the heap...
// According to https://github.com/the-tcpdump-group/libpcap/blob/1131a7c26c6f4d4772e4a2beeaf7212f4dea74ac/pcap.c#L398-L406 ,
// the return value of pcap_next_ex could be greater than 1 for success.
// Let's just make it 1 if it comes bigger than 1.
int pcap_next_ex_escaping(pcap_t *p, uintptr_t pkt_hdr, uintptr_t pkt_data) {
  int ex = pcap_next_ex(p, (struct pcap_pkthdr**)(pkt_hdr), (const u_char**)(pkt_data));
  if (ex > 1) {
    ex = 1;
  }
  return ex;
}

int pcap_offline_filter_escaping(struct bpf_program *fp, uintptr_t pkt_hdr, uintptr_t pkt) {
	return pcap_offline_filter(fp, (struct pcap_pkthdr*)(pkt_hdr), (const u_char*)(pkt));
}

// pcap_wait returns when the next packet is available or the timeout expires.
// Since it uses pcap_get_selectable_fd, it will not work in Windows.
int pcap_wait(pcap_t *p, int msec) {
	struct pollfd fds[1];
	int fd;

	fd = pcap_get_selectable_fd(p);
	if(fd < 0) {
		return fd;
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	if(msec != 0) {
		return poll(fds, 1, msec);
	}

	// block indefinitely if no timeout provided
	return poll(fds, 1, -1);
}

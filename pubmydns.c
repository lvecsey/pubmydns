/*
    Publish a (dynamic) IP address to a server, into an mmap file.
    Copyright (C) 2020  Lester Vecsey

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <stdint.h>
#include <endian.h>
#include <errno.h>
#include <string.h>

#include <inttypes.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/mman.h>

#include "interface_ip.h"

#include "critbit.h"

#include "readfile.h"
#include "writefile.h"

typedef struct __attribute__ ((packed)) {

  uint64_t indexno;

  uint64_t extra;

  unsigned char ipv6[16];
  
} pubmydns_rec;

int pubmydns_show(char *mmap_fn) {

  pubmydns_rec *rp;

  int fd;

  void *m;

  struct stat buf;

  long int num_recs;

  long int recno;

  int retval;
  
  fd = open(mmap_fn, O_RDWR);
  retval = fstat(fd, &buf);
  m = mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (m == MAP_FAILED) {
    perror("mmap");
    return -1;
  }

  num_recs = buf.st_size / sizeof(pubmydns_rec);
  
  for (recno = 0; recno < num_recs; recno++) {

    rp = (pubmydns_rec*) m;
    rp += recno;

    if (!rp->ipv6[0] && !rp->ipv6[1] && !rp->ipv6[2] && !rp->ipv6[3]) {

      break;
      
    }

    {

      uint64_t decoded_indexno;
      uint64_t decoded_extra;

      memcpy(&decoded_indexno, &(rp->indexno), sizeof(uint64_t));
      memcpy(&decoded_extra, &(rp->extra), sizeof(uint64_t));      

      decoded_indexno = be64toh(decoded_indexno);
      decoded_extra = be64toh(decoded_extra);
      
      printf("%"PRIu64"->%u.%u.%u.%u\n", decoded_indexno, rp->ipv6[0], rp->ipv6[1], rp->ipv6[2], rp->ipv6[3]); 

    }
      
  }

  munmap(m, buf.st_size);
  retval = close(fd);

  return 0;
  
}

int main(int argc, char *argv[]) {

  unsigned char public_ipaddress[4];

  int retval;

  char cmd_str[80];

  char *intf;

  char *env_PROTO;

  char buf[32];

  int in_fd, out_fd;

  critbit0_tree dynamic_mapping;

  ssize_t bytes_read;
  ssize_t bytes_written;
  
  env_PROTO = getenv("PROTO");

  if (env_PROTO == NULL) {

    char *intf;

    intf = argc>1 ? argv[1] : NULL;
    
    if (!strncmp(intf, "show", 4)) {

      char *mmap_fn;

      mmap_fn = argc>2 ? argv[2] : NULL;

      printf("Showing mmap_fn %s\n", mmap_fn);
      
      pubmydns_show(mmap_fn);

      return 0;
      
    }

  }
  
  if (env_PROTO != NULL && !strncmp(env_PROTO, "TCP", 3)) {

    char hostname[240];

    char *env_PUBMYDNS_MMAPFN;

    int fd;
    struct stat mbuf;
    void *m;

    long int num_recs;
    
    env_PUBMYDNS_MMAPFN = getenv("PUBMYDNS_MMAPFN");

    if (env_PUBMYDNS_MMAPFN != NULL) {
    
      printf("Running in server mode.\n");

      if (env_PUBMYDNS_MMAPFN == NULL) {
	fprintf(stderr, "%s: Need to specify a valid mmap file in PUBMYDNS_MMAPFN\n", __FUNCTION__);
	return -1;
      }
    
      fd = open(env_PUBMYDNS_MMAPFN, O_RDWR);
      if (fd == -1) {
	perror("open");
	return -1;
      }

      retval = fstat(fd, &mbuf);
      if (retval == -1) {
	perror("fstat");
	return -1;
      }

      m = mmap(NULL, mbuf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
      if (m == MAP_FAILED) {
	perror("mmap");
	return -1;
      }

      num_recs = mbuf.st_size / sizeof(pubmydns_rec);
      
      in_fd = 0;
      out_fd = 1;

      dynamic_mapping = (critbit0_tree) { .root = NULL };
    
      for (;;) {

	fprintf(stderr, "%s: Reading pubmydns command.\n", __FUNCTION__);
	
	bytes_read = readfile(in_fd, buf, 8);
	if (bytes_read != 8) {
	  fprintf(stderr, "%s: bytes_read %ld\n", __FUNCTION__, bytes_read);
	  perror("read command");
	  return -1;
	}

	if (!memcmp(buf, "HOSTNAME", 8)) {

	  fprintf(stderr, "%s: Collecting hostname to overwrite.\n", __FUNCTION__);
	
	}
      
	if (!memcmp(buf, "PUBLICIP", 8)) {

	  uint64_t indexno;

	  uint64_t val;
	
	  fprintf(stderr, "%s: Collecting Public IP.\n", __FUNCTION__);

	  bytes_read = readfile(in_fd, buf, 4);
	  if (bytes_read != 4) {
	    perror("read public_ip");
	    return -1;
	  }

	  bytes_read = readfile(in_fd, &val, sizeof(uint64_t));
	  if (bytes_read != sizeof(uint64_t)) {
	    perror("read indexno");
	    return -1;
	  }

	  indexno = be64toh(val);
	
	  {
	    unsigned char *client_ip;
	    char critbuf[40];
	    client_ip = buf;
	    retval = sprintf(critbuf, "%"PRIu64"->%u.%u.%u.%u", indexno, client_ip[0], client_ip[1], client_ip[2], client_ip[3]);
	    retval = critbit0_insert(&dynamic_mapping, critbuf);
	  }

	  if (indexno < num_recs) {

	    pubmydns_rec *rp;

	    unsigned char *client_ip;\

	    uint64_t encoded_indexno;
	    uint64_t encoded_extra;

	    fprintf(stderr, "%s: Advancing into mmap with unique indexno=%"PRIu64"\n", __FUNCTION__, indexno);
	    
	    rp = (pubmydns_rec*) m;

	    rp += indexno;

	    encoded_indexno = htobe64(indexno);
	    encoded_extra = htobe64(0);
	    
	    memcpy(&(rp->indexno), &encoded_indexno, sizeof(uint64_t));
	    memset(&(rp->extra), 0, sizeof(uint64_t));

	    client_ip = buf;
	    
	    rp->ipv6[0] = client_ip[0];
	    rp->ipv6[1] = client_ip[1];
	    rp->ipv6[2] = client_ip[2];
	    rp->ipv6[3] = client_ip[3];	    

	    fprintf(stderr, "%s: Updated ip address %u.%u.%u.%u into mmap file using unique indexno=%"PRIu64"\n", __FUNCTION__, client_ip[0], client_ip[1], client_ip[2], client_ip[3], indexno);

	    bytes_written = writefile(out_fd, "WROTEIP1", 8);
	    if (bytes_written != 8) {
	      perror("write");
	      return -1;
	    }
	    
	  }

	  else {

	    bytes_written = writefile(out_fd, "FAILURE1", 8);
	    if (bytes_written != 8) {
	      perror("write");
	      return -1;
	    }

	  }
	  
	  
	}

	if (!memcmp(buf, "QUITSRVC", 8)) {

	  fprintf(stderr, "%s: Leaving service.\n", __FUNCTION__);

	  break;

	}
      
      }

      retval = munmap(m, mbuf.st_size);
      if (retval == -1) {
	perror("munmap");
	return -1;
      }

      retval = close(fd);
      if (retval == -1) {
	perror("close");
	return -1;
      }
      
      return 0;

    }

    printf("Running in client mode.\n");

    in_fd = 6;
    out_fd = 7;
    
    intf = argc>1 ? argv[1] : NULL;

    if (intf != NULL) {
    
      printf("Using intf %s\n", intf);
      retval = sprintf(cmd_str, "ifconfig '%s' | grep -i 'inet '", intf);
  
      retval = interface_ip(cmd_str, public_ipaddress);
      if (retval == -1) {
	printf("%s: Could not get public ip address on machine.\n", __FUNCTION__);
	return -1;
      }

      printf("Found public ip address %u.%u.%u.%u\n", public_ipaddress[0], public_ipaddress[1], public_ipaddress[2], public_ipaddress[3]);

      printf("Sending public ip to server.\n");
      
      bytes_written = writefile(out_fd, "PUBLICIP", 8);
      if (bytes_written != 8) {
	perror("write");
	return -1;
      }

      bytes_written = writefile(out_fd, public_ipaddress, 4);
      if (bytes_written != sizeof(public_ipaddress)) {
	perror("write");
	return -1;
      }
      
      {

	uint64_t indexno;

	uint64_t val;

	indexno = argc>2 ? strtol(argv[2], NULL, 10) : 0;

	fprintf(stderr, "%s: Sending unique indexno=%"PRIu64"\n", __FUNCTION__, indexno);
	
	val = htobe64(indexno);

	bytes_written = writefile(out_fd, &val, sizeof(uint64_t));
	if (bytes_written != sizeof(uint64_t)) {
	  perror("write");
	  return -1;
	}
	
      }

      bytes_read = readfile(in_fd, buf, 8);
      if (bytes_read != 8) {
	fprintf(stderr, "%s: bytes_read %ld\n", __FUNCTION__, bytes_read);
	perror("read ack");
	return -1;
      }

      if (memcmp(buf, "WROTEIP1", 8)) {
	printf("Failure to update IP address.\n");
	return -1;
      }

      printf("Successfully updated IP address.\n");
      
      bytes_written = writefile(out_fd, "QUITSRVC", 8);
      if (bytes_written != 8) {
	perror("write");
	return -1;
      }

    }

    printf("Leaving client.\n");
    
    return 0;

  }

  return 0;
    
}

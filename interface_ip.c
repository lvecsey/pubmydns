
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "interface_ip.h"

int interface_ip(char *cmd, char *ip_address) {

  FILE *fp;

  char line[240];

  int retval;

  char *cret;
  
  fp = popen(cmd, "r");

  cret = fgets(line, sizeof(line), fp);
  if (cret == NULL) {
    printf("%s: Trouble with call to fgets.\n", __FUNCTION__);
    return -1;
  }

  {
    long int input_ipaddress[4];
    char *rptr;
    rptr = line;
    while (*rptr == ' ' || *rptr == '\t') {
      rptr++;
    }
    retval = sscanf(rptr, "inet %ld.%ld.%ld.%ld", input_ipaddress+0, input_ipaddress+1, input_ipaddress+2, input_ipaddress+3);

    if (retval != 4) {
      printf("Trouble with scanning for ip address. Input line %s\n", line);
      return -1;
    }
    
    ip_address[0] = input_ipaddress[0];
    ip_address[1] = input_ipaddress[1];
    ip_address[2] = input_ipaddress[2];
    ip_address[3] = input_ipaddress[3];    
    
  }
  
  retval = fclose(fp);
  if (retval == -1) {
    perror("fclose");
    return -1;
  }
  
  return 0;
  
}

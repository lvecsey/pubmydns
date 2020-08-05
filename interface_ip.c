/*
    Extract the IP address from the specified interface using ifconfig
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

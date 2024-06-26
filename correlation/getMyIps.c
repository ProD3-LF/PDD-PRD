/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2012-2023 Applied Communication Sciences
 * (now Peraton Labs Inc.)
 *
 * This software was developed in work supported by the following U.S.
 * Government contracts:
 *
 * HR0011-20-C-0160 HR0011-16-C-0061
 *
 *
 * Any opinions, findings and conclusions or recommendations expressed in
 * this material are those of the author(s) and do not necessarily reflect
 * the views, either expressed or implied, of the U.S. Government.
 *
 * DoD Distribution Statement A
 * Approved for Public Release, Distribution Unlimited
 *
 * DISTAR Case 39809, cleared May 22, 2024 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 */
#include "defs.h"
#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int getMyIps(struct sockaddr_in *ipaddr[],int n){
	int i=0;
	struct ifreq req;
	int fd = socket(PF_INET, SOCK_DGRAM, 0);
	if(fd == -1){
		perror("socket");
		return(-1);
	}
	struct if_nameindex *ifs = if_nameindex();
	if(!ifs) {
		perror("if_nameindex");
		return(-1);
	}
	struct if_nameindex *curif=ifs;
	for(; curif && curif->if_name; curif++) {
		strncpy(req.ifr_name, curif->if_name, IFNAMSIZ);
		req.ifr_name[IFNAMSIZ-1] = 0;
		if (ioctl(fd, SIOCGIFADDR, &req) < 0){continue;}
/*		printf("%s: [%s] %x\n", curif->if_name,
		inet_ntoa(((struct sockaddr_in*) &req.ifr_addr)->sin_addr),
		((struct sockaddr_in*) &req.ifr_addr)->sin_addr);*/
		struct sockaddr_in *t1 = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
		t1->sin_addr = ((struct sockaddr_in*) &req.ifr_addr)->sin_addr;
		ipaddr[i]=t1;
		++i;
		if (i >= n) {
			fprintf(stderr,"too many ipaddrs\n");
			break;
		}
	}
	if_freenameindex(ifs);
	if(close(fd)!=0){perror("close");}
	return i;
}
#define MAXIPS 10
void setMyIp(struct in_addr *ip){
	struct sockaddr_in *ipaddr[MAXIPS];
	int n=getMyIps(ipaddr,MAXIPS);
	if (n<=0){
		 ip->s_addr=0;
		 return;
	}
	*ip=ipaddr[n-1]->sin_addr;
	for (int i=0;i<n;++i){free(ipaddr[i]);}
}


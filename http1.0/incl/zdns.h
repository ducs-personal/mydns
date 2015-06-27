/*
 * zdns.h
 *
 *  Created on: Apr 15, 2015
 *      Author: root
 */

#ifndef ZDNS_ZDNS_H_
#define ZDNS_ZDNS_H_

int zdns(char *domain, char *ipstr, int timeout);
int zdns_srv(char *domain, char *ipstr, char *dns_srv, int timeout);

#endif /* ZDNS_ZDNS_H_ */

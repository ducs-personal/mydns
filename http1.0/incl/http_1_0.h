/*
 * http_1_0.h
 *
 *  Created on: Apr 16, 2015
 *      Author: root
 */

#ifndef GIT_HTTP_1_0_ZHTTP_H_
#define GIT_HTTP_1_0_ZHTTP_H_

int http_1_0_post(char *host, char *uri, char *data, long data_len, int timeout, char **response, long *response_len);

int http_1_0_get(char *host, char *uri, int timeout, char **response, long *response_len);

int http_1_0_download(char *host, char *uri, int timeout, char *file);

int http_1_0_hdrlen(char *resp);

#endif /* GIT_HTTP_1_0_ZHTTP_H_ */

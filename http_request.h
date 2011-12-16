#ifndef __HTTP_REQUEST_H
#define __HTTP_REQUEST_H

typedef struct {

} http_request_header;

typedef struct {
  http_request_header *first_header;
} http_request;

#endif

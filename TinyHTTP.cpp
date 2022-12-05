/*
 * mbed Tiny HTTP Client
 * Copyright (c) 2011 Hiroshi Suga
 * Released under the MIT License: http://mbed.org/license/mit
 */

/** @file
 * @brief Tiny HTTP Client
 */

#include "mbed.h"
//#include "EthernetNetIf.h"
#include "EthernetNetIf/LPC1768/if/eth/EthernetNetIf.h"
//#include "TCPSocket.h"
#include "EthernetNetIf/LPC1768/api/TCPSocket.h"
//#include "DNSRequest.h"
#include "EthernetNetIf/LPC1768/api/DNSRequest.h"
#include "TinyHTTP.h"
#include <ctype.h>


TCPSocket *http;
volatile int tcp_ready, tcp_readable, tcp_writable;
volatile int dns_status;

// Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
int base64enc(const char *input, unsigned int length, char *output, int len) {
  static const char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  unsigned int c, c1, c2, c3;

  if (len < ((((length-1)/3)+1)<<2)) return -1;
  for(unsigned int i = 0, j = 0; i<length; i+=3,j+=4) {
    c1 = ((((unsigned char)*((unsigned char *)&input[i]))));
    c2 = (length>i+1)?((((unsigned char)*((unsigned char *)&input[i+1])))):0;
    c3 = (length>i+2)?((((unsigned char)*((unsigned char *)&input[i+2])))):0;

    c = ((c1 & 0xFC) >> 2);
    output[j+0] = base64[c];
    c = ((c1 & 0x03) << 4) | ((c2 & 0xF0) >> 4);
    output[j+1] = base64[c];
    c = ((c2 & 0x0F) << 2) | ((c3 & 0xC0) >> 6);
    output[j+2] = (length>i+1)?base64[c]:'=';
    c = (c3 & 0x3F);
    output[j+3] = (length>i+2)?base64[c]:'=';
  }
  output[(((length-1)/3)+1)<<2] = '\0';
  return 0;
}

// Copyright (c) 2010 Donatien Garnier (donatiengar [at] gmail [dot] com)
int urlencode(char *str, char *buf, int len) {
  static const char to_hex[] = "0123456789ABCDEF";
//  char *pstr = str, *buf = (char*)malloc(strlen(str) * 3 + 1), *pbuf = buf;
  char *pstr = str, *pbuf = buf;

  if (len < (strlen(str) * 3 + 1)) return -1;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') {
      *pbuf++ = *pstr;
    } else if (*pstr == ' ') {
      *pbuf++ = '+';
    } else { 
      *pbuf++ = '%';
      *pbuf++ = to_hex[(*pstr >> 4) & 0x0f];
      *pbuf++ = to_hex[*pstr & 0x0f];
    }
    pstr++;
  }
  *pbuf = '\0';
  return 0;
}


void isr_http (TCPSocketEvent e) {

#ifdef DEBUG
    printf("tcp(%d)\r\n", e);
#endif
    switch(e) {
    case TCPSOCKET_CONNECTED:
        tcp_ready = 1;
        break;

    case TCPSOCKET_READABLE: //Incoming data
        tcp_readable = 1;
        break;

    case TCPSOCKET_WRITEABLE: //We can send data
        tcp_writable = 1;
        break;

    case TCPSOCKET_CONTIMEOUT:
    case TCPSOCKET_CONRST:
    case TCPSOCKET_CONABRT:
    case TCPSOCKET_ERROR:
    case TCPSOCKET_DISCONNECTED:
        tcp_ready = 0;
        break;
    }
}

void createauth (char *user, char *pwd, char *buf, int len) {
    char tmp[80];

    strncpy(buf, "Authorization: Basic ", len);
    snprintf(tmp, sizeof(tmp), "%s:%s", user, pwd);
    base64enc(tmp, strlen(tmp), &buf[strlen(buf)], len - strlen(buf));
    strncat(buf, "\r\n", len - strlen(buf));
}

void isr_dns (DNSReply r) {

#ifdef DEBUG
    printf("dns(%d)\r\n", r);
#endif
    if (DNS_FOUND) {
        dns_status = 1;
    } else {
        dns_status = -1;
    }
}

int httpRequest (int method, Host *host, char *uri, char *head, char *body) {
    TCPSocketErr err;
    Timer timeout;
    char buf[1500];
    int i, ret = -1;

    http = new TCPSocket;
    tcp_ready = 0;
    tcp_readable = 0;
    tcp_writable = 0;

    http->setOnEvent(isr_http);

    // connect
    if (host->getIp().isNull()) {
        // resolv
        DNSRequest dns;
        dns_status = 0;
        dns.setOnReply(isr_dns);
        if (dns.resolve(host) != DNS_OK) goto exit;
        timeout.reset();
        timeout.start();
        while (timeout.read_ms() < HTTP_TIMEOUT) {
            if (dns_status) break;
            Net::poll();
        }
        timeout.stop();
        if (dns_status <= 0) goto exit;
#ifdef DEBUG
        printf("%s [%d.%d.%d.%d]\r\n", host->getName(), (unsigned char)host->getIp()[0], (unsigned char)host->getIp()[1], (unsigned char)host->getIp()[2], (unsigned char)host->getIp()[3]);
#endif
    }
    if (! host->getPort()) {
        host->setPort(HTTP_PORT);
    }
    err = http->connect(*host);
    if (err != TCPSOCKET_OK) goto exit;

    // wait connect
    timeout.reset();
    timeout.start();
    while (timeout.read_ms() < HTTP_TIMEOUT) {
        if (tcp_ready) break;
        Net::poll();
    }
    timeout.stop();
    if (! tcp_ready) goto exit;

    // send request
    if (method == METHOD_POST) {
        http->send("POST ", 5);
    } else {
        http->send("GET ", 4);
    }
    http->send(uri, strlen(uri));
    http->send(" HTTP/1.1\r\nHost: ", 17);
    http->send(host->getName(), strlen(host->getName()));
    http->send("\r\n", 2);
    http->send("Connection: close\r\n", 19);
    if (head) {
        http->send(head, strlen(head));
    }
    if (method == METHOD_POST) {
        sprintf(buf, "Content-Length: %d\r\n", strlen(body));
        http->send(buf, strlen(buf));
    }
    http->send("\r\n", 2);

    // post method
    if (method == METHOD_POST && body) {
        http->send(body, strlen(body));
    }

    // wait responce
    timeout.reset();
    timeout.start();
    while (timeout.read_ms() < HTTP_TIMEOUT) {
        if (tcp_readable) break;
        Net::poll();
    }
    timeout.stop();
    if (! tcp_readable) goto exit;

    // recv responce
    i = http->recv(buf, sizeof(buf) - 1);
    buf[i] = 0;
    if (i < sizeof(buf) - 1) tcp_readable = 0;
    if (strncmp(buf, "HTTP/", 5) == 0) {
        ret = atoi(&buf[9]);
    }
#ifdef DEBUG
    printf(buf);
#endif

    // recv dummy
    timeout.reset();
    timeout.start();
    while (timeout.read_ms() < HTTP_TIMEOUT) {
        if (tcp_readable) {
            i = http->recv(buf, sizeof(buf) - 1);
            buf[i] = 0;
            if (i < sizeof(buf) - 1) tcp_readable = 0;
#ifdef DEBUG
            printf(buf);
#endif
            timeout.reset();
        } else
        if (! tcp_ready) {
            break;
        }
        Net::poll();
    }
    timeout.stop();

exit:
    http->resetOnEvent();
    http->close();
    delete http;

    return ret;
}

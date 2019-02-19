#ifndef _E32_API_H
#define _E32_API_H

int e32Config();

int e32_start();

int e32_listen();

void e32_send(uint8_t *payload, uint8_t nb_byte);

void e32_fetch( uint8_t data_len, uint8_t* pdata );

#endif
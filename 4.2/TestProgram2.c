#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../base_code/Cache.h"

// Declarações das funções que estão no Cache.h ou outro lugar apropriado
unsigned int getTime(void);  // Declarando a função getTime
void resetTime(void);        // Declarando a função resetTime
void write(uint32_t address, uint8_t* data);  // Declaração da função write
void read(uint32_t address, uint8_t* data);   // Declaração da função read

// Função para imprimir os dados, endereços e tempo
void print_data(const char* operation, uint32_t address, uint8_t* data) {
    printf("%s - Address: 0x%08X, Data: ", operation, address);
    for (int i = 0; i < WORD_SIZE; i++) {
        printf("%d ", data[i]);
    }
    printf(", Time: %u\n", getTime());  // Chamada à função getTime
}

// Teste para verificar write-back entre dois endereços que caem na mesma linha da cache
void test_write_same_cache_line() {
    uint8_t data1[WORD_SIZE] = {1, 1, 1, 1};  // Dados para escrever
    uint8_t data2[WORD_SIZE] = {2, 2, 2, 2};  // Outros dados para escrever
    uint8_t read_data[WORD_SIZE] = {0};       // Buffer para leitura

    // Dois endereços diferentes que mapeiam para o mesmo index na cache
    uint32_t address1 = 0x0000;  // Tag 0, Index 0
    uint32_t address2 = 0x4000;  // Tag 1, Index 0 (mesmo index)

    resetTime();  // Reinicia o tempo

    // Escreve no primeiro endereço
    write(address1, data1);  // Usando a função write correta
    print_data("WRITE", address1, data1);

    // Escreve no segundo endereço, deve ocorrer um write-back do primeiro endereço
    write(address2, data2);  // Usando a função write correta
    print_data("WRITE", address2, data2);

    // Lê do segundo endereço para verificar se o write-back foi correto
    read(address2, read_data);  // Usando a função read correta
    print_data("READ", address2, read_data);

    // Agora vamos ler do primeiro endereço, deve ocorrer um miss e buscar no L2
    read(address1, read_data);  // Usando a função read correta
    print_data("READ", address1, read_data);
}

// Teste para verificar write-back seguido de leitura
void test_write_back_and_read() {
    uint8_t data1[WORD_SIZE] = {5, 5, 5, 5};  // Dados para escrever
    uint8_t read_data[WORD_SIZE] = {0};       // Buffer para leitura

    // Dois endereços diferentes que mapeiam para o mesmo index na cache
    uint32_t address1 = 0x0000;  // Tag 0, Index 0
    uint32_t address2 = 0x4000;  // Tag 1, Index 0 (mesmo index)

    resetTime();  // Reinicia o tempo

    // Escreve no primeiro endereço
    write(address1, data1);  // Usando a função write correta
    print_data("WRITE", address1, data1);

    // Agora escreve em outro endereço com o mesmo index, forçando write-back
    write(address2, data1);  // Write-back de address1 deve ocorrer
    print_data("WRITE", address2, data1);

    // Agora lê de address1, deve vir do L2 após o write-back
    read(address1, read_data);  // Usando a função read correta
    print_data("READ", address1, read_data);
}

int main() {
    // Executa os testes
    printf("Test 1: Write in Same Cache Line\n");
    test_write_same_cache_line();

    printf("\nTest 2: Write Back and Read\n");
    test_write_back_and_read();

    return 0;
}

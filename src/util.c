#include "util.h"

#include <stdlib.h>
#include <string.h>

uint64_t read_v0_file(struct era_t *era, FILE *executable)
{
	uint32_t length = 0;
	size_t read = 0;

	// Skip the padding
	fseek(executable, 1, SEEK_CUR);
	if(ferror(executable) != 0 || feof(executable) != 0)
	{
		return READ_ERROR_READ;
	}

	// Load the length
	fread((void*)&length, sizeof(uint32_t), 1, executable);
	if(ferror(executable) != 0 || feof(executable) != 0)
	{
		return READ_ERROR_READ;
	}

	// Load the static data and the code
	read = fread((void*) era->memory, sizeof(word_t), MEM_SIZE, executable);
	// We CAN get EOF here, but errors are still possible
	if(ferror(executable) != 0)
	{
		return READ_ERROR_READ;
	}

	// Deal with little-endianess
	if(little_endian() == 1)
	{
		length = swap_lword(length);
		for(size_t c = 0; c < read; ++c)
		{
			era->memory[c] = swap_word(era->memory[c]);
		}
	}

	// Populate PC
	// I was a bit dumb at first.
	// length relates to the length of data in the global data + code, NOT in the file.
	// We don't need to modify it
	era->registers[PC] = length;

	return 0;
}

uint64_t read_v1_file(struct era_t *era, FILE *executable)
{
	uint32_t data_start;
	uint32_t data_length;
	uint32_t code_start;
	uint32_t code_length;
	word_t *code;
	word_t *data;
	size_t read;

	// Skip the padding
	fseek(executable, 1, SEEK_CUR);
	if(ferror(executable) != 0 || feof(executable) != 0)
	{
		return READ_ERROR_READ;
	}

	// Read info about static data
	fread((void*)&data_start, sizeof(uint32_t), 1, executable);
	if(ferror(executable) != 0 || feof(executable) != 0 || data_start == 0)
	{
		return READ_ERROR_READ;
	}
	fread((void*)&data_length, sizeof(uint32_t), 1, executable);
	if(ferror(executable) != 0 || feof(executable) != 0 || data_length == 0)
	{
		return READ_ERROR_READ;
	}

	// Read info about code
	fread((void*)&code_start, sizeof(uint32_t), 1, executable);
	if(ferror(executable) != 0 || feof(executable) != 0 || code_start == 0)
	{
		return READ_ERROR_READ;
	}
	fread((void*)&code_length, sizeof(uint32_t), 1, executable);
	if(ferror(executable) != 0 || feof(executable) != 0 || code_length == 0)
	{
		return READ_ERROR_READ;
	}

	// Deal with little-endianess
	if(little_endian() == 1)
	{
		code_start = swap_lword(code_start);
		code_length = swap_lword(code_length);
		data_start = swap_lword(data_start);
		data_length = swap_lword(data_length);
	}

	code = (word_t*)malloc(code_length);
	data = (word_t*)malloc(data_length);

	// TODO: Make sure this always works
	// TODO: Could get the length of a file and additionally check against it
	fseek(executable, data_start, SEEK_SET);
	read = fread(data, sizeof(word_t), data_length, executable);
	if(ferror(executable) != 0 || feof(executable) != 0 || read != data_length)
	{
		return READ_ERROR_READ;
	}


	fseek(executable, code_start, SEEK_SET);
	read = fread(code, sizeof(word_t), code_length, executable);
	if(ferror(executable) != 0 || feof(executable) != 0 || read != code_length)
	{
		return READ_ERROR_READ;
	}

	// Copy the data into the ERA memory
	memcpy(era->memory, data, data_length * sizeof(word_t));
	// TODO: Not sure about this one. Makes sense, but looks dangerous
	memcpy(era->memory + data_length, code, code_length * sizeof(word_t));

	// TODO: Should work
	era->registers[PC] = data_length;

	// Deal with little-endianess
	if(little_endian() == 1)
	{
		for(size_t c = 0; c < data_length + code_length; ++c)
		{
			era->memory[c] = swap_word(era->memory[c]);
		}
	}

	free(data);
	free(code);

	return 0;
}

sword_t read_sword(struct era_t *era, lword_t address)
{
	if(address > MEM_SIZE)
		return 0;
	return (sword_t)(era->memory[address] & 0xFF);
}

word_t read_word(struct era_t *era, lword_t address)
{
	if(address > MEM_SIZE)
		return 0;
	return era->memory[address];
}

lword_t read_lword(struct era_t *era, lword_t address)
{
	if(address + 1 > MEM_SIZE)
		return 0;
	// sizeof(word_t) * 8 returns number of bits
	return (lword_t)(era->memory[address] << (sizeof(word_t) * 8) | era->memory[address + 1]);
}

int write_lword(struct era_t *era, lword_t address, lword_t word)
{
	// TODO: Add dynamic memory sizing
	if(address > MEM_SIZE || address + 1 > MEM_SIZE)
		return 1;

	era->memory[address] = (word_t)((word >> 16) & 0xffff);
	era->memory[address + 1] = (word_t)(word & 0xffff);

	return 0;
}

uint8_t little_endian()
{
	// Little-endian:
	// 01 00
	// Big endian:
	// 00 01
	// Therfore, 1 will be read on little-endian and 0 on big-endian
	uint16_t x = 1;
	return *(uint8_t *) &x;
}

word_t swap_word(word_t word)
{
	return (word_t)(((word & 255) << 8) + ((word >> 8) & 255));
}

lword_t swap_lword(lword_t word)
{
	sword_t p1, p2, p3, p4;
	p1 = word & 255;
	p2 = (word >> 8) & 255;
	p3 = (word >> 16) & 255;
	p4 = (word >> 24) & 255;
	return (lword_t)((p1 << 24) + (p2 << 16) + (p3 << 8) + p4);
}

#include<memory.h>
#include<stdio.h>

#define GLO_NULL						0
#define HSTUP_FRAME_DUMP_POOL_SIZE		12288
typedef unsigned int	u32;
typedef char			u8;
typedef u32				TPriorityQueueIndex;

u8 frame_content[18] = {
						0x00,
						0x11,
						0x22,
						0x33,
						0x44,
						0x55,
						0x66,
						0x77,
						0x88,
						0x99,
						0xaa,
						0xbb,
						0xcc,
						0xdd,
						0xee,
						0xff,
						0xff,
						0xff
						};

char* file_name = "dump_memory_result.bin";
						
//----------------------------------------------------------------
//code start
typedef struct
{
	u32 pool_header_start;
	u32 dump_frame_length;
	u32 slot_length;
	u32 max_slot_num;
	u32 current_slot;
	u32 pool_header_end;
	u32 dynamic[1];
}dump_pool_header_t;

typedef struct
{
	u32 frame_header_start;
	TPriorityQueueIndex queueIndex;
	u32 payload_crc;
	u32 payload_address;
	u32 frame_header_end;
	u32 payload_content[1];
}frame_header_t;

#define END_OF_FRAME_TAG		0xEEDDAA55
#define END_OF_FRAME_TAG_LEN	4
#define HEADER_START_MARK		0xAABBAA55
#define HEADER_END_MARK			0x0E0FAA55
u8 hstup_frame_dump_pool_200_bits[HSTUP_FRAME_DUMP_POOL_SIZE] = {0};
#define get_pool_header(pool)					(dump_pool_header_t*)pool
#define get_frame_header(start, slot, len)		(frame_header_t*)((u8*)start + slot*len)

void hstup_frame_dump_pool_init(const u8* pool_start, u32 frame_size)
{
	if(GLO_NULL == pool_start)
		return;
	u32 frame_size_in_bytes = (frame_size + 7) >> 3;
	dump_pool_header_t* pool_header = get_pool_header(pool_start);
	pool_header->pool_header_start = HEADER_START_MARK;
	pool_header->pool_header_end = HEADER_END_MARK;
	pool_header->dump_frame_length = frame_size_in_bytes;
	pool_header->slot_length = (sizeof(frame_header_t) + frame_size_in_bytes - sizeof(u32) + END_OF_FRAME_TAG_LEN + 3) & ~0x3;
	pool_header->current_slot = 0;
	pool_header->max_slot_num = (HSTUP_FRAME_DUMP_POOL_SIZE - sizeof(dump_pool_header_t) + sizeof(u32))/pool_header->slot_length;
}

void hstup_frame_dump_pool_init_once()
{
	hstup_frame_dump_pool_init(hstup_frame_dump_pool_200_bits, 200);
}

u8* hstup_allocate_dump_memory(const u8* hstup_frame_dump_pool)
{
	if(GLO_NULL == hstup_frame_dump_pool)
		return GLO_NULL;
	frame_header_t* frame_header = GLO_NULL;
	dump_pool_header_t* pool_header = get_pool_header(hstup_frame_dump_pool);
	frame_header = get_frame_header(pool_header->dynamic, pool_header->current_slot, pool_header->slot_length);	/*lint !e613 reason: it is ok*/
	pool_header->current_slot++;   /*lint !e613 reason: it is ok*/
	if(pool_header->current_slot >= pool_header->max_slot_num)
	{
		pool_header->current_slot = 0; /*lint !e613 reason: it is ok*/
	}
	return (u8*)frame_header->payload_content;
}

//unit of frame size: bits
void hstup_dump_frame_content(u32 frame_size, frame_header_t* frame_header, const u8* frame_content)
{
	if(!frame_header || !frame_content)
		return;
	u32 frame_size_in_bytes = (((frame_size + 7)>>3) + 3) & ~0x3;
	u8* dump_slot_ptr = GLO_NULL;
	if(frame_size < 200)
	{
		dump_slot_ptr = hstup_allocate_dump_memory(hstup_frame_dump_pool_200_bits);
	}
	else
	{
		//TODO
	}

	if(dump_slot_ptr)
	{
		frame_header_t* temp_frame_header = (frame_header_t*)(dump_slot_ptr - sizeof(frame_header_t) + sizeof(u32));
		temp_frame_header->frame_header_start = HEADER_START_MARK;
		temp_frame_header->frame_header_end = HEADER_END_MARK;
		temp_frame_header->queueIndex = frame_header->queueIndex;
		temp_frame_header->payload_crc = frame_header->payload_crc;
		temp_frame_header->payload_address = frame_header->payload_address;
		memcpy((void*)dump_slot_ptr, (void*)frame_content, frame_size_in_bytes);
		u32* temp_end_mark_ptr = (u32*)(dump_slot_ptr + frame_size_in_bytes);
		*temp_end_mark_ptr = (u32)END_OF_FRAME_TAG;
	}
	else
	{
		//TODO
	}
}

//code end
//----------------------------------------------------------------

int main(int argc, char** argv)
{
	hstup_frame_dump_pool_init_once();
	int i;
	for(i = 0; i < 300; i++)
	{
		frame_header_t in_frame_header;
		in_frame_header.queueIndex = i;
		in_frame_header.payload_crc = 0xccfc;
		in_frame_header.payload_address = (u32)frame_content;
		hstup_dump_frame_content(144, &in_frame_header, frame_content);
	}
	
	FILE* file_ptr;
	file_ptr = fopen(file_name, "wb");
	if(GLO_NULL != file_ptr)
	{
		fwrite(hstup_frame_dump_pool_200_bits, 122888, 1, file_ptr);
		fclose(file_ptr);
	}
	return 0;
}

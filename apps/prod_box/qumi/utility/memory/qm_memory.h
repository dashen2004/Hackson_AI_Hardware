/** @file    qm_memory.h
 *  @author  Wells
 *  @version 1.0
 *  @date    25-Apr-2016
 *  @brief  
 */

#ifndef _QM_MEMORY_H_
#define _QM_MEMORY_H_
#include "serial_common.h"

#ifdef	__cplusplus
	extern "C" {
#endif

/* Block sizes must not get too small. */
#define qm_heapMINIMUM_BLOCK_SIZE	( ( uint32_t ) ( xHeapStructSize * 2 ) )

/* Assumes 8bit bytes! */
#define qm_heapBITS_PER_BYTE		( ( uint32_t ) 8 )



#define qm_portBYTE_ALIGNMENT			4

#if qm_portBYTE_ALIGNMENT == 8
#define qm_portBYTE_ALIGNMENT_MASK ( 0x0007U )
#endif

#if qm_portBYTE_ALIGNMENT == 4
#define qm_portBYTE_ALIGNMENT_MASK	( 0x0003 )
#endif

#if qm_portBYTE_ALIGNMENT == 2
#define qm_portBYTE_ALIGNMENT_MASK	( 0x0001 )
#endif

#if qm_portBYTE_ALIGNMENT == 1
#define qm_portBYTE_ALIGNMENT_MASK	( 0x0000 )
#endif

void qm_heap_init( uint8_t* heap, uint32_t heap_len);
void *qm_malloc( uint32_t xWantedSize );
void qm_free( void *pv );
void *qm_calloc(uint32_t num, uint32_t size);
uint32_t qm_free_mem_get( void );


#ifdef	__cplusplus
	}
#endif
	

#endif

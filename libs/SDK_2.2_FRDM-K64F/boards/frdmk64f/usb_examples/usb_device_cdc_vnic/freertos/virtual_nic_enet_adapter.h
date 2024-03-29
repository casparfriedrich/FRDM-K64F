/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _VIRTUAL_NIC_ENET_ADAPTER_H_
#define _VIRTUAL_NIC_ENET_ADAPTER_H_ 1

#include "virtual_nic.h"
/*******************************************************************************
* Definitions
******************************************************************************/
/* USB header size of RNDIS packet in bytes. */
#define RNDIS_USB_OVERHEAD_SIZE (44)
/* Specifies the offset in bytes from the start of the DataOffset field of rndis_packet_msg_struct_t to the start of the
 * data. */
#define RNDIS_DATA_OFFSET (36)

/* ENET queue size. */
#define ENET_QUEUE_MAX (16)

/* Define VNIC_ENET transfer struct */
typedef struct _vnic_enet_transfer
{
    uint8_t *buffer;
    uint32_t length;
} vnic_enet_transfer_t;

/* Define VNIC_ENET queue struct */
typedef struct _queue
{
    uint32_t head;
    uint32_t tail;
    uint32_t maxSize;
    uint32_t curSize;
    usb_osa_mutex_handle mutex;
    vnic_enet_transfer_t *qArray;
} queue_t;

/* Alloc variable for primask. */
#define USB_DEVICE_VNIC_SR_ALLOC() \
    \
uint8_t sr = 0

/* Alloc variable for primask. */
#define USB_DEVICE_VNIC_CRITICAL_ALLOC() USB_DEVICE_VNIC_SR_ALLOC()

/* Eneter critical section. */
#define USB_DEVICE_VNIC_ENTER_CRITICAL() \
    \
VNIC_EnetEnterCritical(&sr)

/* Exit critical section. */
#define USB_DEVICE_VNIC_EXIT_CRITICAL() \
    \
VNIC_EnetExitCritical(sr)

/*******************************************************************************
* API
******************************************************************************/
extern volatile uint32_t g_vnicEnterCriticalCnt;
/*!
 * @brief Disable interrupt to enter critical section.
 *
 * @return The value of PRIMASK register before disable irq.
 */
static inline void VNIC_EnetEnterCritical(uint8_t *sr)
{
    if (__get_IPSR())
    {
        *sr = portSET_INTERRUPT_MASK_FROM_ISR();
    }
    else
    {
        portENTER_CRITICAL();
    }
}

/*!
 * @brief Enable interrupt to exit critical section.
 *
 * @return None.
 */
static inline void VNIC_EnetExitCritical(uint8_t sr)
{
    if (__get_IPSR())
    {
        portCLEAR_INTERRUPT_MASK_FROM_ISR(sr);
    }
    else
    {
        portEXIT_CRITICAL();
    }
}

/*!
 * @brief Initialize the queue.
 *
 * @return Error code.
 */
static inline usb_status_t VNIC_EnetQueueInit(queue_t *q, uint32_t maxSize)
{
    usb_status_t error = kStatus_USB_Error;
    USB_DEVICE_VNIC_CRITICAL_ALLOC();
    USB_DEVICE_VNIC_ENTER_CRITICAL();
    (q)->head = 0;
    (q)->tail = 0;
    (q)->maxSize = maxSize;
    (q)->curSize = 0;
    if (kStatus_USB_OSA_Success != USB_OsaMutexCreate(&((q)->mutex)))
    {
        usb_echo("queue mutex create error!");
    }
    error = kStatus_USB_Success;
    USB_DEVICE_VNIC_EXIT_CRITICAL();
    return error;
}

/*!
 * @brief Put element into the queue.
 *
 * @return Error code.
 */
static inline usb_status_t VNIC_EnetQueuePut(queue_t *q, vnic_enet_transfer_t *e)
{
    usb_status_t error = kStatus_USB_Error;
    USB_DEVICE_VNIC_CRITICAL_ALLOC();
    USB_DEVICE_VNIC_ENTER_CRITICAL();
    if ((q)->curSize < (q)->maxSize)
    {
        (q)->qArray[(q)->head++] = *(e);
        if ((q)->head == (q)->maxSize)
        {
            (q)->head = 0;
        }
        (q)->curSize++;
        error = kStatus_USB_Success;
    }
    USB_DEVICE_VNIC_EXIT_CRITICAL();
    return error;
}

/*!
 * @brief Get element from the queue.
 *
 * @return Error code.
 */
static inline usb_status_t VNIC_EnetQueueGet(queue_t *q, vnic_enet_transfer_t *e)
{
    usb_status_t error = kStatus_USB_Error;
    USB_DEVICE_VNIC_CRITICAL_ALLOC();
    USB_DEVICE_VNIC_ENTER_CRITICAL();
    if ((q)->curSize)
    {
        *(e) = (q)->qArray[(q)->tail++];
        if ((q)->tail == (q)->maxSize)
        {
            (q)->tail = 0;
        }
        (q)->curSize--;
        error = kStatus_USB_Success;
    }
    USB_DEVICE_VNIC_EXIT_CRITICAL();
    return error;
}

/*!
 * @brief Delete the queue.
 *
 * @return Error code.
 */
static inline usb_status_t VNIC_EnetQueueDelete(queue_t *q)
{
    usb_status_t error = kStatus_USB_Error;
    USB_DEVICE_VNIC_CRITICAL_ALLOC();
    USB_DEVICE_VNIC_ENTER_CRITICAL();
    (q)->head = 0;
    (q)->tail = 0;
    (q)->maxSize = 0;
    (q)->curSize = 0;
    error = kStatus_USB_Success;
    USB_DEVICE_VNIC_EXIT_CRITICAL();
    return error;
}

/*!
 * @brief Check if the queue is empty.
 *
 * @return 1: queue is empty, 0: not empty.
 */
static inline uint8_t VNIC_EnetQueueIsEmpty(queue_t *q)
{
    return ((q)->curSize == 0) ? 1 : 0;
}

/*!
 * @brief Check if the queue is full.
 *
 * @return 1: queue is full, 0: not full.
 */
static inline uint8_t VNIC_EnetQueueIsFull(queue_t *q)
{
    return ((q)->curSize >= (q)->maxSize) ? 1 : 0;
}

/*!
 * @brief Get the size of the queue.
 *
 * @return Size of the quue.
 */
static inline uint32_t VNIC_EnetQueueSize(queue_t *q)
{
    return (q)->curSize;
}

/*!
 * @brief Initialize the ethernet module.
 *
 * @return Error code.
 */
extern uint32_t VNIC_EnetInit(void);

/*!
 * @brief Clear the transfer requests in enet queue.
 *
 * @return Error code.
 */
extern usb_status_t VNIC_EnetClearEnetQueue(void);

/*!
 * @brief Send packets to Ethernet module.
 *
 * @return Error code.
 */
#endif /* _VIRTUAL_NIC_ENET_ADAPTER_H_ */

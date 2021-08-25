/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *        Copyright 2014, 2015 Big Switch Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 *
 *
 ***********************************************************/
#include <onlplib/i2c.h>

#if ONLPLIB_CONFIG_INCLUDE_I2C == 1

#include <onlplib/file.h>
#include <fcntl.h>
#include <unistd.h>

#if ONLPLIB_CONFIG_I2C_USE_CUSTOM_HEADER == 1
#include <linux/i2c-devices.h>
#else
#include <linux/i2c-dev.h>
#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <onlp/onlp.h>
#include "onlplib_log.h"

int
onlp_i2c_open(int bus, uint8_t addr, uint32_t flags)
{
    int fd;
    int rv;

    fd = onlp_file_open(O_RDWR, 1, "/dev/i2c-%d", bus);
    if(fd < 0) {
        return fd;
    }

    /* Set 10 or 7 bit mode */
    rv = ioctl(fd, I2C_TENBIT, (flags & ONLP_I2C_F_TENBIT) ? 1 : 0);
    if(rv == -1) {
        AIM_LOG_ERROR("i2c-%d: failed to set %d bit mode", bus,
                      (flags & ONLP_I2C_F_TENBIT) ? 10 : 7);
        goto error;
    }

    /* Enable/Disable PEC */
    rv = ioctl(fd, I2C_PEC, (flags & ONLP_I2C_F_PEC) ? 1 : 0);
    if(rv == -1) {
        AIM_LOG_ERROR("i2c-%d: failed to set PEC mode %d", bus,
                      (flags & ONLP_I2C_F_PEC) ? 1 : 0);
        goto error;
    }

    /* Set SLAVE or SLAVE_FORCE address */
    rv = ioctl(fd,
               (flags & ONLP_I2C_F_FORCE) ? I2C_SLAVE_FORCE : I2C_SLAVE,
               addr);

    if(rv == -1) {
        AIM_LOG_ERROR("i2c-%d: %s slave address 0x%x failed: %{errno}",
                      bus,
                      (flags & ONLP_I2C_F_FORCE) ? "forcing" : "setting",
                      addr,
                      errno);
        goto error;
    }

    return fd;

 error:
    close(fd);
    return ONLP_STATUS_E_I2C;
}

int
onlp_i2c_block_read(int bus, uint8_t addr, uint8_t offset, int size,
                    uint8_t* rdata, uint32_t flags)
{
    int fd;

    fd = onlp_i2c_open(bus, addr, flags);

    if(fd < 0) {
        return fd;
    }

    int count = size;
    uint8_t* p = rdata;
    while(count > 0) {
        int rsize = (count >= ONLPLIB_CONFIG_I2C_BLOCK_SIZE) ? ONLPLIB_CONFIG_I2C_BLOCK_SIZE : count;
        int retries = (flags & ONLP_I2C_F_DISABLE_READ_RETRIES) ? 1 : ONLPLIB_CONFIG_I2C_READ_RETRY_COUNT;

        int rv = -1;
        while(retries-- && rv < 0) {
            if(flags & ONLP_I2C_F_USE_SMBUS_BLOCK_READ) {
                rv = i2c_smbus_read_block_data(fd, offset, p);
            } else {
                rv = i2c_smbus_read_i2c_block_data(fd,
                                                   offset,
                                                   rsize,
                                                   p);
            }
            if(rv >= 0) {
                offset += rsize;
            }
        }

        if(rv != rsize) {
            AIM_LOG_ERROR("i2c-%d: reading address 0x%x, offset %d, size=%d failed: %{errno}",
                          bus, addr, p - rdata, rsize, errno);
            goto error;
        }

        p += rsize;
        count -= rsize;
    }

    close(fd);
    return 0;

 error:
    close(fd);
    return ONLP_STATUS_E_I2C;
}

int
onlp_i2c_read(int bus, uint8_t addr, uint8_t offset, int size,
              uint8_t* rdata, uint32_t flags)
{
    int i;
    int fd;

    fd = onlp_i2c_open(bus, addr, flags);

    if(fd < 0) {
        return fd;
    }

    for(i = 0; i < size; i++) {
        int rv = -1;
        int retries = (flags & ONLP_I2C_F_DISABLE_READ_RETRIES) ? 1: ONLPLIB_CONFIG_I2C_READ_RETRY_COUNT;

        while(retries-- && rv < 0) {
            rv = i2c_smbus_read_byte_data(fd, offset+i);
        }

        if(rv < 0) {
            AIM_LOG_ERROR("i2c-%d: reading address 0x%x, offset %d failed: %{errno}",
                          bus, addr, offset+i, errno);
            goto error;
        }
        else {
            rdata[i] = rv;
        }
    }
    close(fd);
    return 0;

 error:
    close(fd);
    return ONLP_STATUS_E_I2C;
}


int
onlp_i2c_write(int bus, uint8_t addr, uint8_t offset, int size,
               uint8_t* data, uint32_t flags)
{
    int i;
    int fd;

    fd = onlp_i2c_open(bus, addr, flags);

    if(fd < 0) {
        return fd;
    }

    for(i = 0; i < size; i++) {
        int rv = i2c_smbus_write_byte_data(fd, offset+i, data[i]);
        if(rv < 0) {
            AIM_LOG_ERROR("i2c-%d: writing address 0x%x, offset %d failed: %{errno}",
                          bus, addr, offset+i, errno);
            goto error;
        }
    }
    close(fd);
    return 0;

 error:
    close(fd);
    return ONLP_STATUS_E_I2C;
}

int
onlp_i2c_readb(int bus, uint8_t addr, uint8_t offset, uint32_t flags)
{
    uint8_t byte;
    int rv = onlp_i2c_read(bus, addr, offset, 1, &byte, flags);
    return (rv < 0) ? rv : byte;
}

int
onlp_i2c_writeb(int bus, uint8_t addr, uint8_t offset, uint8_t byte,
                uint32_t flags)
{
    return onlp_i2c_write(bus, addr, offset, 1, &byte, flags);
}

int
onlp_i2c_modifyb(int bus, uint8_t addr, uint8_t offset,
                 uint8_t andmask, uint8_t ormask, uint32_t flags)
{
    int v;
    ONLP_IF_ERROR_RETURN(v=onlp_i2c_readb(bus, addr, offset, flags));
    v &= andmask;
    v |= ormask;
    return onlp_i2c_writeb(bus, addr, offset, v, flags);
}


int
onlp_i2c_readw(int bus, uint8_t addr, uint8_t offset, uint32_t flags)
{
    int fd;
    int rv;

    fd = onlp_i2c_open(bus, addr, flags);

    if(fd < 0) {
        return fd;
    }

    rv = i2c_smbus_read_word_data(fd, offset);

    close(fd);
    return rv;
}

int
onlp_i2c_writew(int bus, uint8_t addr, uint8_t offset, uint16_t word,
                    uint32_t flags)
{
    int fd;
    int rv;

    fd = onlp_i2c_open(bus, addr, flags);

    if(fd < 0) {
        return fd;
    }

    rv = i2c_smbus_write_word_data(fd, offset, word);

    close(fd);
    return rv;

}

int
onlp_i2c_mux_select(onlp_i2c_mux_device_t* dev, int channel)
{
    int i;
    for(i = 0; i < AIM_ARRAYSIZE(dev->driver->channels); i++) {
        if(dev->driver->channels[i].channel == channel) {
            AIM_LOG_VERBOSE("i2c_mux_select: Selecting channel %2d on device '%s'  [ bus=%d addr=0x%x offset=0x%x value=0x%x ]...",
                        channel, dev->name, dev->bus, dev->devaddr,
                        dev->driver->control, dev->driver->channels[i].value);

            int rv = onlp_i2c_writeb(dev->bus,
                                     dev->devaddr,
                                     dev->driver->control,
                                     dev->driver->channels[i].value,
                                     0);

            if(rv < 0) {
                AIM_LOG_ERROR("i2c_mux_select: Selecting channel %2d on device '%s'  [ bus=%d addr=0x%x offset=0x%x value=0x%x ] failed: %d",
                              channel, dev->name, dev->bus, dev->devaddr,
                              dev->driver->control, dev->driver->channels[i].value, rv);
            }
            return rv;
        }
    }
    return ONLP_STATUS_E_PARAM;
}


int
onlp_i2c_mux_deselect(onlp_i2c_mux_device_t* dev)
{
    return onlp_i2c_mux_select(dev, -1);
}


int
onlp_i2c_mux_channel_select(onlp_i2c_mux_channel_t* mc)
{
    return onlp_i2c_mux_select(mc->mux, mc->channel);
}


int
onlp_i2c_mux_channel_deselect(onlp_i2c_mux_channel_t* mc)
{
    return onlp_i2c_mux_deselect(mc->mux);
}


int
onlp_i2c_mux_channels_select(onlp_i2c_mux_channels_t* mcs)
{
    int i;
    for(i = 0; i < AIM_ARRAYSIZE(mcs->channels); i++) {
        if(mcs->channels[i].mux) {
            int rv = onlp_i2c_mux_channel_select(mcs->channels + i);
            if(rv < 0) {
                /** Error already reported */
                return rv;
            }
        }
    }
    return 0;
}


int
onlp_i2c_mux_channels_deselect(onlp_i2c_mux_channels_t* mcs)
{
    int i;
    for(i = AIM_ARRAYSIZE(mcs->channels) - 1; i >= 0; i--) {
        if(mcs->channels[i].mux) {
            int rv = onlp_i2c_mux_channel_deselect(mcs->channels + i);
            if(rv < 0) {
                /** Error already reported. */
                return rv;
            }
        }
    }
    return 0;
}


int
onlp_i2c_dev_mux_channels_select(onlp_i2c_dev_t* dev)
{
    if(dev->pchannels) {
        return onlp_i2c_mux_channels_select(dev->pchannels);
    }
    else {
        return onlp_i2c_mux_channels_select(&dev->ichannels);
    }
}


int
onlp_i2c_dev_mux_channels_deselect(onlp_i2c_dev_t* dev)
{
    if(dev->pchannels) {
        return onlp_i2c_mux_channels_deselect(dev->pchannels);
    }
    else {
        return onlp_i2c_mux_channels_deselect(&dev->ichannels);
    }
}


static int
dev_mux_channels_select__(onlp_i2c_dev_t* dev, uint32_t flags)
{
    if(flags & ONLP_I2C_F_NO_MUX_SELECT) {
        return 0;
    }
    return onlp_i2c_dev_mux_channels_select(dev);
}

static int
dev_mux_channels_deselect__(onlp_i2c_dev_t* dev, uint32_t flags)
{
    if(flags & ONLP_I2C_F_NO_MUX_DESELECT) {
        return 0;
    }
    return onlp_i2c_dev_mux_channels_deselect(dev);
}


int
onlp_i2c_dev_read(onlp_i2c_dev_t* dev, uint8_t offset, int size,
                  uint8_t* rdata, uint32_t flags)
{
    int error, rv;

    if( (error = dev_mux_channels_select__(dev, flags)) < 0) {
        return error;
    }

    if(flags & ONLP_I2C_F_USE_BLOCK_READ) {
        rv = onlp_i2c_block_read(dev->bus, dev->addr, offset, size, rdata, flags);
    }
    else {
        rv = onlp_i2c_read(dev->bus, dev->addr, offset, size, rdata, flags);
    }

    if( rv < 0 ) {
        AIM_LOG_ERROR("Device %s: read() failed: %d",
                      dev->name, rv);
        return rv;
    }

    if( (error = dev_mux_channels_deselect__(dev, flags)) < 0) {
        return error;
    }

    return rv;
}


int
onlp_i2c_dev_write(onlp_i2c_dev_t* dev,
                   uint8_t offset, int size, uint8_t* data, uint32_t flags)
{
    int error, rv;

    if( (error = dev_mux_channels_select__(dev, flags)) < 0) {
        return error;
    }

    if( (rv = onlp_i2c_write(dev->bus, dev->addr, offset, size, data, flags)) < 0) {
        AIM_LOG_ERROR("Device %s: write() failed: %d",
                      dev->name, rv);
        return rv;
    }

    if( (error = dev_mux_channels_deselect__(dev, flags)) < 0) {
        return error;
    }

    return rv;
}


int
onlp_i2c_dev_readb(onlp_i2c_dev_t* dev, uint8_t offset, uint32_t flags)
{
    int error, rv;

    if( (error = dev_mux_channels_select__(dev, flags)) < 0) {
        return error;
    }

    if( (rv = onlp_i2c_readb(dev->bus, dev->addr, offset, flags)) < 0) {
        AIM_LOG_ERROR("Device %s: readb() failed: %d",
                      dev->name, rv);
        return rv;
    }

    if( (error = dev_mux_channels_deselect__(dev, flags)) < 0) {
        return error;
    }

    return rv;
}


int
onlp_i2c_dev_writeb(onlp_i2c_dev_t* dev,
                    uint8_t offset, uint8_t byte, uint32_t flags)
{
    int error, rv;

    if( (error = dev_mux_channels_select__(dev, flags)) < 0) {
        return error;
    }

    if( (rv = onlp_i2c_writeb(dev->bus, dev->addr, offset, byte, flags)) < 0) {
        AIM_LOG_ERROR("Device %s: writeb() failed: %d",
                      dev->name, rv);
        return rv;
    }

    if( (error = dev_mux_channels_deselect__(dev, flags)) < 0) {
        return error;
    }

    return rv;
}


int
onlp_i2c_dev_readw(onlp_i2c_dev_t* dev,
                   uint8_t offset, uint32_t flags)
{
    int error, rv;

    if( (error = dev_mux_channels_select__(dev, flags)) < 0) {
        return error;
    }

    if( (rv = onlp_i2c_readw(dev->bus, dev->addr, offset, flags)) < 0) {
        AIM_LOG_ERROR("Device %s: readw() failed: %d",
                      dev->name, rv);
        return rv;
    }

    if( (error = dev_mux_channels_deselect__(dev, flags)) < 0) {
        return error;
    }

    return rv;
}


int
onlp_i2c_dev_writew(onlp_i2c_dev_t* dev,
                        uint8_t offset, uint16_t word, uint32_t flags)
{
    int error, rv;

    if( (error = dev_mux_channels_select__(dev, flags)) < 0) {
        return error;
    }

    if( (rv = onlp_i2c_writew(dev->bus, dev->addr, offset, word, flags)) < 0) {
        AIM_LOG_ERROR("Device %s: writew() failed: %d",
                      dev->name, rv);
        return rv;
    }

    if( (error = dev_mux_channels_deselect__(dev, flags)) < 0) {
        return error;
    }

    return rv;
}

/**
 * PCA9547A
 */
onlp_i2c_mux_driver_t onlp_i2c_mux_driver_pca9547a =
    {
        .name = "PCA9547A",
        .control = 0,
        .channels =
        {
            { -1,   0 },
            {  0, 0x8 },
            {  1, 0x9 },
            {  2, 0xA },
            {  3, 0xB },
            {  4, 0xC },
            {  5, 0xD },
            {  6, 0xE },
            {  7, 0xF },
        },
    };

/**
 * PCA9548
 */
onlp_i2c_mux_driver_t onlp_i2c_mux_driver_pca9548 =
    {
        .name = "PCA9548A",
        .control = 0,
        .channels =
        {
            { -1, 0x00 },
            {  0, 0x01 },
            {  1, 0x02 },
            {  2, 0x04 },
            {  3, 0x08 },
            {  4, 0x10 },
            {  5, 0x20 },
            {  6, 0x40 },
            {  7, 0x80 },
        },
    };

/* I2C port mapping for BF6064X-T */
int
onlp_i2c_mux_mapping(int port_number, int reset)
{
    // Check if reset = 0 (do the mapping) or 1 (reset the registers). Exit otherwise
    if (reset != 0 && reset != 1) {
        AIM_LOG_ERROR("reset can only be 1 (reset) or 0 (mapping)");
        return 0;
    }

    int channel_cpu; // cpu channel number from hardware specification
    int channel_mb; // mb channel number from hardware specification
    onlp_i2c_mux_device_t cpu_mux;
    onlp_i2c_mux_device_t mb_mux;
    cpu_mux.name = "CPU MUX";
    cpu_mux.bus = 0;
    cpu_mux.devaddr = 0x70; //CPU MUX address from hardware specification
    cpu_mux.driver = &onlp_i2c_mux_driver_pca9548; //Driver for BF6064X switch
    mb_mux.name = "MB MUX";
    mb_mux.bus = 0;
    mb_mux.driver = &onlp_i2c_mux_driver_pca9548; //For BF6064X switch

    if (port_number > 0 && port_number <= 32) {
        channel_cpu = 2;
    } else if (port_number >= 33 && port_number <= 64) {
        channel_cpu = 6;
    } else {
        AIM_LOG_ERROR("i2c_mux_mapping: Port number %d is out of limits", port_number);
    }

    // Select or deselect CPU MUX
    int rv;
    if (reset == 0) {
        rv = onlp_i2c_mux_select(&cpu_mux, channel_cpu);
        if(rv < 0) {
            return rv;
        }
    } else { // reset = 1
        rv = onlp_i2c_mux_deselect(&cpu_mux);
        if(rv < 0) {
            return rv;
        }
    }

    switch (port_number) {
        case 1:
            mb_mux.devaddr = 0x74;
            channel_mb = 0;
            break;
        case 33:
            mb_mux.devaddr = 0x74;
            channel_mb = 0;
            break;
        case 2:
            mb_mux.devaddr = 0x74;
            channel_mb = 1;
            break;
        case 34:
            mb_mux.devaddr = 0x74;
            channel_mb = 1;
            break;
        case 3:
            mb_mux.devaddr = 0x74;
            channel_mb = 2;
            break;
        case 35:
            mb_mux.devaddr = 0x74;
            channel_mb = 2;
            break;
        case 4:
            mb_mux.devaddr = 0x74;
            channel_mb = 3;
            break;
        case 36:
            mb_mux.devaddr = 0x74;
            channel_mb = 3;
            break;
        case 5:
            mb_mux.devaddr = 0x74;
            channel_mb = 4;
            break;
        case 37:
            mb_mux.devaddr = 0x74;
            channel_mb = 4;
            break;
        case 6:
            mb_mux.devaddr = 0x74;
            channel_mb = 5;
            break;
        case 38:
            mb_mux.devaddr = 0x74;
            channel_mb = 5;
            break;
        case 7:
            mb_mux.devaddr = 0x74;
            channel_mb = 6;
            break;
        case 39:
            mb_mux.devaddr = 0x74;
            channel_mb = 6;
            break;
        case 8:
            mb_mux.devaddr = 0x74;
            channel_mb = 7;
            break;
        case 40:
            mb_mux.devaddr = 0x74;
            channel_mb = 7;
            break;
        case 14:
            mb_mux.devaddr = 0x75;
            channel_mb = 0;
            break;
        case 47:
            mb_mux.devaddr = 0x75;
            channel_mb = 0;
            break;
        case 13:
            mb_mux.devaddr = 0x75;
            channel_mb = 1;
            break;
        case 48:
            mb_mux.devaddr = 0x75;
            channel_mb = 1;
            break;
        case 16:
            mb_mux.devaddr = 0x75;
            channel_mb = 2;
            break;
        case 41:
            mb_mux.devaddr = 0x75;
            channel_mb = 2;
            break;
        case 15:
            mb_mux.devaddr = 0x75;
            channel_mb = 3;
            break;
        case 42:
            mb_mux.devaddr = 0x75;
            channel_mb = 3;
            break;
        case 10:
            mb_mux.devaddr = 0x75;
            channel_mb = 4;
            break;
        case 43:
            mb_mux.devaddr = 0x75;
            channel_mb = 4;
            break;
        case 9:
            mb_mux.devaddr = 0x75;
            channel_mb = 5;
            break;
        case 44:
            mb_mux.devaddr = 0x75;
            channel_mb = 5;
            break;
        case 12:
            mb_mux.devaddr = 0x75;
            channel_mb = 6;
            break;
        case 45:
            mb_mux.devaddr = 0x75;
            channel_mb = 6;
            break;
        case 11:
            mb_mux.devaddr = 0x75;
            channel_mb = 7;
            break;
        case 46:
            mb_mux.devaddr = 0x75;
            channel_mb = 7;
            break;
        case 17:
            mb_mux.devaddr = 0x76;
            channel_mb = 0;
            break;
        case 55:
            mb_mux.devaddr = 0x76;
            channel_mb = 0;
            break;
        case 18:
            mb_mux.devaddr = 0x76;
            channel_mb = 1;
            break;
        case 56:
            mb_mux.devaddr = 0x76;
            channel_mb = 1;
            break;
        case 26:
            mb_mux.devaddr = 0x76;
            channel_mb = 2;
            break;
        case 53:
            mb_mux.devaddr = 0x76;
            channel_mb = 2;
            break;
        case 25:
            mb_mux.devaddr = 0x76;
            channel_mb = 3;
            break;
        case 54:
            mb_mux.devaddr = 0x76;
            channel_mb = 3;
            break;
        case 22:
            mb_mux.devaddr = 0x76;
            channel_mb = 4;
            break;
        case 58:
            mb_mux.devaddr = 0x76;
            channel_mb = 4;
            break;
        case 21:
            mb_mux.devaddr = 0x76;
            channel_mb = 5;
            break;
        case 57:
            mb_mux.devaddr = 0x76;
            channel_mb = 5;
            break;
        case 24:
            mb_mux.devaddr = 0x76;
            channel_mb = 6;
            break;
        case 49:
            mb_mux.devaddr = 0x76;
            channel_mb = 6;
            break;
        case 23:
            mb_mux.devaddr = 0x76;
            channel_mb = 7;
            break;
        case 50:
            mb_mux.devaddr = 0x76;
            channel_mb = 7;
            break;
        case 29:
            mb_mux.devaddr = 0x77;
            channel_mb = 0;
            break;
        case 59:
            mb_mux.devaddr = 0x77;
            channel_mb = 0;
            break;
        case 30:
            mb_mux.devaddr = 0x77;
            channel_mb = 1;
            break;
        case 60:
            mb_mux.devaddr = 0x77;
            channel_mb = 1;
            break;
        case 27:
            mb_mux.devaddr = 0x77;
            channel_mb = 2;
            break;
        case 61:
            mb_mux.devaddr = 0x77;
            channel_mb = 2;
            break;
        case 28:
            mb_mux.devaddr = 0x77;
            channel_mb = 3;
            break;
        case 62:
            mb_mux.devaddr = 0x77;
            channel_mb = 3;
            break;
        case 32:
            mb_mux.devaddr = 0x77;
            channel_mb = 4;
            break;
        case 63:
            mb_mux.devaddr = 0x77;
            channel_mb = 4;
            break;
        case 31:
            mb_mux.devaddr = 0x77;
            channel_mb = 5;
            break;
        case 64:
            mb_mux.devaddr = 0x77;
            channel_mb = 5;
            break;
        case 19:
            mb_mux.devaddr = 0x77;
            channel_mb = 6;
            break;
        case 52:
            mb_mux.devaddr = 0x77;
            channel_mb = 6;
            break;
        case 20:
            mb_mux.devaddr = 0x77;
            channel_mb = 7;
            break;
        default: //or case 51:
            mb_mux.devaddr = 0x77;
            channel_mb = 7;
            break;
    }

    // Select or deselect MB MUX
    if (reset == 0) {
        return onlp_i2c_mux_select(&mb_mux, channel_mb);
    } else {
        return onlp_i2c_mux_deselect(&mb_mux);
    }
}


#endif /* ONLPLIB_CONFIG_INCLUDE_I2C */

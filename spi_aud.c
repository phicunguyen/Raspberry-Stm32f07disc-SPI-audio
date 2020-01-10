#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>


#define IN  				0
#define OUT 				1
#define LOW  				0
#define HIGH 				1
#define PIN  				5 /* P1-29 */
#define BUFFER_MAX 			3
#define DIRECTION_MAX 			35
#define VALUE_MAX 			30
#define SERIAL_PACKET__AUDIO_PLAY 	4

#define AUDIO_BUFFER_SIZE		1024
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const char *device = "/dev/spidev0.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 1600000;
static uint16_t delay1;
uint8_t buffer[1032];
int fd;

static void gpio_export(int pin) {
    int fd;
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    fd = open("/sys/class/gpio/export", O_WRONLY);
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
}

static void gpio_unexport(int pin) {
    int fd;
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
}

static void  gpio_direction(int pin, int dir) {
    int fd;
    static const char s_directions_str[]  = "in\0out";
    char path[DIRECTION_MAX];
    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
    fd = open(path, O_WRONLY);
    write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3);
    close(fd);
}

static int gpio_read(int pin) {
    int fd;
    char path[VALUE_MAX];
    char value_str[3];
    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    read(fd, value_str, 3);
    close(fd);
    return(atoi(value_str));
}

static void gpio_write(int pin, int value) {
    int fd;
    static const char s_values_str[] = "01";
    char path[VALUE_MAX];
    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    write(fd, &s_values_str[LOW == value ? 0 : 1], 1);
    close(fd);
}


static void spi_write(uint8_t *pbuf, uint16_t len)
{
    struct spi_ioc_transfer tr = {
	.tx_buf = (unsigned long)pbuf,
	.len = len,
	.delay_usecs = delay1,
	.speed_hz = speed,
	.bits_per_word = bits,
    };

    ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
}

void spi_init(void)
{
    int ret = 0;

    fd = open(device, O_RDWR);
    ioctl(fd, SPI_IOC_WR_MODE, &mode);
    ioctl(fd, SPI_IOC_RD_MODE, &mode);
    ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);

    printf("spi mode: %d\n", mode);
    printf("bits per word: %d\n", bits);
    printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

}

static void s_sync_stuffing(uint8_t **pbuf, uint8_t *pdata, uint16_t size) {
    uint8_t *p_data = *pbuf;
    for (int i = 0; i < size; i++) {
	if (pdata[i] == 0x5A) {
	    *p_data++ = pdata[i];
	    *p_data++ = 0;
	} else if (pdata[i] == 0x5B) { //'[' 0x5B
	    *p_data++ = pdata[i]-1;
	    *p_data++ = 1;
	} else if (pdata[i] == 0x5C){
	    *p_data++ = pdata[i];
            *p_data++ = 0;
	} else if (pdata[i] == 0x5D){ //']' 0x5D
	    *p_data++ = pdata[i]-1;
            *p_data++ = 1;
	} else {
	    *p_data++ = pdata[i];
	}
    }
    *pbuf = p_data;
}

static void s_write(uint8_t *p_buf, uint16_t size) {
    uint8_t *p_data = buffer;
    //start of packet
    *p_data++ = '[';
    s_sync_stuffing(&p_data, p_buf, size);
    //end of packet
    *p_data++ = ']';
    spi_write(buffer, (uint16_t)(p_data - buffer));
}

static void is_data_ready(void) {
    while (gpio_read(PIN) == 0);
}

int main(int argc, char *argv[]) {
    FILE *fptr;
    uint32_t size=44;
    uint32_t cnt=0;
    uint8_t buf[AUDIO_BUFFER_SIZE+2];

    gpio_export(PIN);
    gpio_direction(PIN, IN);
    spi_init();
    fptr = fopen("audio.wav", "rb");
    buf[0] = SERIAL_PACKET__AUDIO_PLAY;
    buf[1] = 0x00;
 
    while (!feof(fptr)) { 
        size = fread(&buf[2], sizeof(buf[0]), size, fptr);
        //printf("\nsize (%d) :%d\n", cnt++,  size);
        s_write(buf, size+2);
	if (size == 44)    
	   size = AUDIO_BUFFER_SIZE;
        is_data_ready();
    }
    gpio_unexport(PIN);
    close(fd);
}

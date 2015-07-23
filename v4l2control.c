#define V4L2CONTROL_C
#include "v4l2control.h"
#include <stdbool.h>

#define GRAB_DEVICE "/dev/video0"
static char* grab_dev = GRAB_DEVICE;
#define CLEAR(x) memset(&(x), 0, sizeof (x))
int GRAB_WIDTH = 1024, GRAB_HEIGHT = 1024;

typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;

struct buffer {
	void   *start;
	size_t length;
};
static io_method io = IO_METHOD_USERPTR;
struct buffer * buffers = NULL;
static unsigned int n_buffers = 0;

static int  grab_fd=-1;

static void errno_exit(const char * s){
	fprintf(stderr, "%s: error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

static int xioctl(int grab_fd, int request, void * arg){
	int r;
	do r = ioctl(grab_fd, request, arg);
	while(-1 == r && EINTR == errno);
	return r;
}

/*
		Работа с платой захвата
*/
static bool init_read(unsigned int buffer_size){
	buffers = calloc(1, sizeof (*buffers));
	if (!buffers){
		ERR("init_read: Out of memory");
		return 0;
	}
	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);
	if (!buffers[0].start){
		ERR("init_read: Out of memory");
		return 0;
	}
	WARN("\ninit_read, buf size = %d\n", buffer_size);
	return 1;
}

static bool init_mmap(unsigned int *bufsize){
	struct v4l2_requestbuffers req;
	CLEAR (req);
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if(-1 == xioctl (grab_fd, VIDIOC_REQBUFS, &req)){
		if(EINVAL == errno){
			ERR("%s does not support memory mapping\n", grab_dev);
			return 0;
		}else{
			ERR("VIDIOC_REQBUFS");
			return 0;
		}
	}
	if(req.count < 2){
		ERR("Insufficient buffer memory on %s\n", grab_dev);
		return 0;
	}
	buffers = calloc(req.count, sizeof (*buffers));
	if(!buffers){
		ERR("Out of memory");
		return 0;
	}
	for(n_buffers = 0; n_buffers < req.count; ++n_buffers){
		struct v4l2_buffer buf;
		CLEAR (buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
		if(-1 == xioctl(grab_fd, VIDIOC_QUERYBUF, &buf)){
			ERR("VIDIOC_QUERYBUF");
			return 0;
		}
		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
		mmap(NULL, // start anywhere
			buf.length,
			PROT_READ | PROT_WRITE, // required
			MAP_SHARED, // recommended
			grab_fd, buf.m.offset);
		if(MAP_FAILED == buffers[n_buffers].start){
			ERR("mmap");
			return 0;
		}
	}
	WARN("\ninit_mmap(), nbuffers=%d, buf size=%d\n", sizeof (*buffers), buffers[0].length);
	return 1;
}

static bool init_userp(unsigned int buffer_size){
	struct v4l2_requestbuffers req;
	unsigned int page_size;
	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);
	CLEAR (req);
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
	if(-1 == xioctl(grab_fd, VIDIOC_REQBUFS, &req)){
		if(EINVAL == errno){
			ERR("%s does not support user pointer i/o\n", grab_dev);
			return 0;
		}else{
			ERR("VIDIOC_REQBUFS");
			return 0;
		}
	}
	buffers = calloc(4, sizeof (*buffers));
	if(!buffers){
		ERR("Out of memory");
		return 0;
	}
	for(n_buffers = 0; n_buffers < 4; ++n_buffers){
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = memalign(page_size,	buffer_size);
		if(!buffers[n_buffers].start){
			ERR("Out of memory");
			return 0;
		}
	}
	WARN("\ninit_userp, buf size=%d, n buffers = %d\n", buffer_size, n_buffers);
	return 1;
}

// сис. вызовы для подготовки к захвату (~3мс)
static void start_capturing (void){
	unsigned int i;
	enum v4l2_buf_type type;
	switch(io){
	case IO_METHOD_READ:
		// Nothing to do.
		break;

	case IO_METHOD_MMAP:
		for(i = 0; i < n_buffers; ++i){
			struct v4l2_buffer buf;
			CLEAR (buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;
			if(-1 == xioctl(grab_fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(-1 == xioctl(grab_fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
		break;

	case IO_METHOD_USERPTR:
		for(i = 0; i < n_buffers; ++i){
			struct v4l2_buffer buf;
			CLEAR (buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long) buffers[i].start;
			buf.length = buffers[i].length;
			if(-1 == xioctl(grab_fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if(-1 == xioctl(grab_fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
		break;
	}
	WARN("\nstart_capturing()\n");
}

// Настройки захвата (~2мс)
unsigned int grab_init(char *dev, int chnum){
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	int i = 0;
	bool ret = 0;
	unsigned int bufsize;
	if(dev) grab_dev = strdup(dev);
	if(grab_fd < 0){ // при первой инициализации открываем
		struct stat st; 
		if(-1 == stat(grab_dev, &st)){
			ERR("Cannot identify '%s': %d, %s\n", grab_dev, errno, strerror (errno));
			exit (EXIT_FAILURE);
		}
		if(!S_ISCHR(st.st_mode)){
			ERR("%s is no device\n", grab_dev);
			exit(EXIT_FAILURE);
		}
		grab_fd = open(grab_dev, O_RDWR | O_NONBLOCK, 0);
		if(-1 == grab_fd){
			ERR("Cannot open '%s': %d, %s\n", grab_dev, errno, strerror (errno));
			exit(EXIT_FAILURE);
		}
	}
	if(-1 == xioctl(grab_fd, VIDIOC_QUERYCAP, &cap)){
		if(EINVAL == errno){
			ERR("%s is not V4L2 device\n", grab_dev);
			exit(EXIT_FAILURE);
		} else
			errno_exit ("VIDIOC_QUERYCAP");
	}
	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
		ERR("%s is no video capture device\n", grab_dev);
		exit (EXIT_FAILURE);
	}
	
	
	
	
	
struct v4l2_queryctrl queryctrl;
struct v4l2_querymenu querymenu;
struct v4l2_control control;

void
enumerate_menu (void)
{
        printf ("  Menu items:\n");

        CLEAR(querymenu);
        querymenu.id = queryctrl.id;

        for (querymenu.index = queryctrl.minimum;
             querymenu.index <= queryctrl.maximum;
              querymenu.index++) {
                if (0 == ioctl (grab_fd, VIDIOC_QUERYMENU, &querymenu)) {
                        printf ("  %s\n", querymenu.name);
                } else {
                        perror ("VIDIOC_QUERYMENU");
                        exit (EXIT_FAILURE);
                }
        }
}

printf("\n\nCONTROLS AND THEIR VALUES\n");
CLEAR(queryctrl);
for (queryctrl.id = V4L2_CID_BASE;
     queryctrl.id < V4L2_CID_LASTP1;
     queryctrl.id++) {
        if (0 == ioctl (grab_fd, VIDIOC_QUERYCTRL, &queryctrl)) {
                if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                        continue;

                printf ("Control %s ", queryctrl.name);
				CLEAR(control);
				control.id = queryctrl.id;
				if (0 == ioctl (grab_fd, VIDIOC_G_CTRL, &control)) {
					printf("num: %d (value: %d", control.id-V4L2_CID_BASE, control.value);
				}
				else printf("err");
				printf("; max: %d, min: %d, step: %d, def: %d)\n",
					queryctrl.maximum, queryctrl.minimum, queryctrl.step,
						queryctrl.default_value);
                if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
                        enumerate_menu ();
        } else {
                if (errno == EINVAL)
                        continue;

                perror ("VIDIOC_QUERYCTRL");
                exit (EXIT_FAILURE);
        }
}

for (queryctrl.id = V4L2_CID_PRIVATE_BASE;;
     queryctrl.id++) {
        if (0 == ioctl (grab_fd, VIDIOC_QUERYCTRL, &queryctrl)) {
                if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                        continue;

                printf ("Control %s\n", queryctrl.name);

                if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
                        enumerate_menu ();
        } else {
                if (errno == EINVAL)
                        break;

                perror ("VIDIOC_QUERYCTRL");
                exit (EXIT_FAILURE);
        }
}

printf("\n\n");

/* Отключение автоусиления:
        CLEAR(control);
        control.id = V4L2_CID_AUTOGAIN;
        control.value = 0;
        if (-1 == ioctl (grab_fd, VIDIOC_S_CTRL, &control)) {
                perror ("VIDIOC_S_CTRL");
                exit (EXIT_FAILURE);
        }
*/

	
/* для тюнера:
	struct v4l2_standard argp;
	CLEAR(argp);
	argp.index=0;
	if(-1 == xioctl(grab_fd, VIDIOC_ENUMSTD, &argp)){
		if(EINVAL == errno) ERR("bad index: %d\n", argp.index);
	}
	else{
		fprintf(stderr, "STANDARDS SUPPORTED (%o): ", argp.id);
		if(argp.id & 0xFFF) fprintf(stderr, "PAL ");
		if(argp.id & 0xFFF000) fprintf(stderr, "SECAM ");
		if(argp.id & 0x3000000) fprintf(stderr, "ATSC/HDTV");
		fprintf(stderr, "\n");
	}
*/
	
	for(i = 0; i<2; i++){
		switch(io){
		case IO_METHOD_READ:
			if (!(cap.capabilities & V4L2_CAP_READWRITE)){
				ERR("%s does not support read i/o, trying streaming\n", grab_dev);
				io = IO_METHOD_MMAP;
			}
			else{
				WARN("grab_init: IO_METHOD_READ\n");
				i = 2;
			}
			break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
				ERR("%s does not support streaming i/o, trying reading\n", grab_dev);
				io = IO_METHOD_READ;
			}
			else{
				WARN("grab_init: IO_METHOD_USERPTR / IO_METHOD_MMAP\n");
				i = 2;
			}
			break;
		}
	}
	// Select video input, video standard and tune here.
	grab_set_chan(chnum);
	CLEAR (cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(0 == xioctl (grab_fd, VIDIOC_CROPCAP, &cropcap)){
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; // reset to default 
		if(-1 == xioctl(grab_fd, VIDIOC_S_CROP, &crop)){
			switch (errno){
			case EINVAL:
				WARN("Cropping not supported\n");
			break;
			default:
				WARN("VIDIOC_S_CROP: error\n");
			break;
			}
		}
	} else { 
		WARN("VIDIOC_CROPCAP error\n");
	}
	CLEAR (fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = GRAB_WIDTH; 
	fmt.fmt.pix.height = GRAB_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;	
	if(xioctl(grab_fd, VIDIOC_TRY_FMT, &fmt) == 0){
		if(fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) printf("\nRGB фигвам\n");
	}
	
	CLEAR (fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = GRAB_WIDTH; 
	fmt.fmt.pix.height = GRAB_HEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; //YUYV;
//	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24; //LD_PRELOAD=/usr/lib/libv4l/v4l2convert.so ./wavinfo

	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED; //INTERLACED;
	if(-1 == xioctl(grab_fd, VIDIOC_S_FMT, &fmt))
		errno_exit("VIDIOC_S_FMT");
	// Note VIDIOC_S_FMT may change width and height:
	GRAB_WIDTH = fmt.fmt.pix.width;
	GRAB_HEIGHT = fmt.fmt.pix.height;
	// Buggy driver paranoia:
/*	minwd = fmt.fmt.pix.width * 3;
	if (fmt.fmt.pix.bytesperline < minwd)
		fmt.fmt.pix.bytesperline = minwd;
	minwd = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < minwd)
		fmt.fmt.pix.sizeimage = minwd;*/
	bufsize = fmt.fmt.pix.sizeimage;
	for(i=0; i<3; i++){ // пробуем все методы, если начальный не заработает
		switch(io){
		case IO_METHOD_READ:
			if(ret = init_read(fmt.fmt.pix.sizeimage)) i = 3;
			else io = IO_METHOD_MMAP;
			break;
		case IO_METHOD_MMAP:
			if(ret = init_mmap(&bufsize)) i = 3;
			else io = IO_METHOD_USERPTR;
			break;
		case IO_METHOD_USERPTR:
			if(ret = init_userp(fmt.fmt.pix.sizeimage)) i = 3;
			else io = IO_METHOD_READ;
			break;
		}
	}
	if(!ret){
		errno_exit("No methods available");
	}
	WARN("grab_init(): format=%dx%d\n", GRAB_WIDTH, GRAB_HEIGHT);
	start_capturing();
	return bufsize;
}

// активация канала захвата
void grab_set_chan(int ch_num){
	if(-1 == ioctl(grab_fd, VIDIOC_S_INPUT, &ch_num))
		errno_exit("VIDIOC_S_INPUT");
	WARN("grab_set_chan(%d)\n", ch_num);
}

void change_controls(int brightness, int contrast){
	struct v4l2_queryctrl queryctrl;
	struct v4l2_control control;
	if(brightness != 0){
		memset(&queryctrl, 0, sizeof(queryctrl));
		queryctrl.id = V4L2_CID_BRIGHTNESS;
		if(-1 == ioctl(grab_fd, VIDIOC_QUERYCTRL, &queryctrl)){
			if(errno != EINVAL){
				perror("VIDIOC_QUERYCTRL");
				exit(EXIT_FAILURE);
			}else
				ERR("V4L2_CID_BRIGHTNESS is not supported\n");
		}else if(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
			ERR("V4L2_CID_BRIGHTNESS disabled\n");
		else{
			memset(&control, 0, sizeof (control));
			control.id = V4L2_CID_BRIGHTNESS;
			control.value = brightness; //queryctrl.default_value;
			if(-1 == ioctl(grab_fd, VIDIOC_S_CTRL, &control))
				errno_exit("VIDIOC_S_CTRL");
		}
	}
	if(contrast != 0){
	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_CONTRAST;
	if(0 == ioctl(grab_fd, VIDIOC_G_CTRL, &control)){
		control.value = contrast;
	/* The driver may clamp the value or return ERANGE, ignored here */
		if(-1 == ioctl(grab_fd, VIDIOC_S_CTRL, &control) && errno != ERANGE){
			perror("VIDIOC_S_CTRL");
			exit(EXIT_FAILURE);
		}
	/* Ignore if V4L2_CID_CONTRAST is unsupported */
	} else if(errno != EINVAL) {
		perror("VIDIOC_G_CTRL");
		exit(EXIT_FAILURE);
	}
	}
}

unsigned char* grab_next(){
	struct v4l2_buffer buf;
	unsigned int i = 0;
	void* ret;
	fd_set fds;
	struct timeval tv;
	int r;
	for(;;){
		FD_ZERO(&fds);
		FD_SET(grab_fd, &fds);
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		r = select(grab_fd + 1, &fds, NULL, NULL, &tv);
		if(-1 == r){
			if(EINTR == errno)
				continue;
			errno_exit ("select");
		}
		if(0 == r){
			errno_exit("select timeout\n");
		}
		switch(io){
		case IO_METHOD_READ:
			if(-1 == read(grab_fd, buffers[0].start, buffers[0].length)){
				switch(errno){
					case EAGAIN:
						return NULL;
					case EIO:
						ERR("EIO");
						break;
					default:
					errno_exit("read");
				}
			}
			ret = buffers[0].start;
			break;

		case IO_METHOD_MMAP:
			CLEAR (buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			if(-1 == xioctl(grab_fd, VIDIOC_DQBUF, &buf)){
				switch(errno){
					case EAGAIN:
						return NULL;
					case EIO:
						ERR("EIO");
						break;
					default:
						errno_exit("VIDIOC_DQBUF");
				}
			}
			assert(buf.index < n_buffers);
			ret = buffers[buf.index].start;
			if(-1 == xioctl(grab_fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
			break;

		case IO_METHOD_USERPTR:
			CLEAR (buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			if(-1 == xioctl(grab_fd, VIDIOC_DQBUF, &buf)){
				switch (errno){
					case EAGAIN:
						return NULL;
					case EIO:
						ERR("EIO");
						break;
					default:
					errno_exit("VIDIOC_DQBUF");
				}
			}
			for(i = 0; i < n_buffers; ++i)
				if(buf.m.userptr == (unsigned long) buffers[i].start
					&& buf.length == buffers[i].length)
					break;
			assert(i < n_buffers);
			ret = (void *) buf.m.userptr;
			if(-1 == xioctl(grab_fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
			break;
		}
		if (ret != NULL) break;
	}
/*
	int file = open("image.jpg", O_RDWR|O_CREAT);
	write(file, ret, buffers[i].length);
	close(file);
*/
//	WARN("grab_next, size = %d\n", buffers[i].length);
	return (unsigned char*) ret;
}


/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
#include <jpeglib.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

typedef enum {
	IO_METHOD_READ, IO_METHOD_MMAP, IO_METHOD_USERPTR,
} io_method;

struct buffer {
	void *start;
	size_t length;
};

static char *dev_name;
/* static io_method io = IO_METHOD_USERPTR; */
static io_method io = IO_METHOD_MMAP;
static int fd = -1;
struct buffer *buffers;
static unsigned int n_buffers;
static unsigned char *yuv420buf;

#define CAPTURE_FILE_YUYV "/home/orangepi/test_picture1.yuyv"
#define CAPTURE_FILE_YUV "/home/orangepi/test_picture2.yuv"
#define CAPTURE_FILE_JPG "/home/orangepi/test_picture3.jpg"

static void errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));

	exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void *arg)
{
	int r;

	do {
		r = ioctl(fd, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

static int write_JPEG_file(char *filename, unsigned char *yuvData,
				int quality, int image_width, int image_height)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *outfile;    /* target file */
	JSAMPROW row_pointer[1];  /* pointer to JSAMPLE row[s] */
	int row_stride;    /* physical row width in image buffer */
	JSAMPIMAGE buffer;
	unsigned char *pSrc, *pDst;
	int band, i, buf_width[3], buf_height[3], counter;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	outfile = fopen(filename, "wb");
	if (outfile == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		exit(1);
	}

	jpeg_stdio_dest(&cinfo, outfile);

	/* image width and height, in pixels */
	cinfo.image_width = image_width;
	cinfo.image_height = image_height;
	cinfo.input_components = 3;    /* of color components per pixel */
	cinfo.in_color_space = JCS_RGB;  /* colorspace of input image */

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	cinfo.raw_data_in = TRUE;
	cinfo.jpeg_color_space = JCS_YCbCr;
	cinfo.comp_info[0].h_samp_factor = 2;
	cinfo.comp_info[0].v_samp_factor = 2;

	jpeg_start_compress(&cinfo, TRUE);
	buffer = (JSAMPIMAGE)(*cinfo.mem->alloc_small)((j_common_ptr)&cinfo,
	JPOOL_IMAGE, 3 * sizeof(JSAMPARRAY));

	for (band = 0; band < 3; band++) {
		buf_width[band] = cinfo.comp_info[band].width_in_blocks * DCTSIZE;
		buf_height[band] = cinfo.comp_info[band].v_samp_factor * DCTSIZE;
		buffer[band] = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo,
		JPOOL_IMAGE, buf_width[band], buf_height[band]);
	}

	unsigned char *rawData[3];
	int src_width[3], src_height[3];

	rawData[0] = yuvData;
	rawData[1] = yuvData + image_width * image_height;
	rawData[2] = yuvData + image_width * image_height * 5 / 4;

	for (i = 0; i < 3; i++) {
		src_width[i] = (i == 0) ? image_width : image_width / 2;
		src_height[i] = (i == 0) ? image_height : image_height / 2;
	}

	int max_line = cinfo.max_v_samp_factor * DCTSIZE;
	for (counter = 0; cinfo.next_scanline < cinfo.image_height; counter++) {
		/* buffer image copy. */
		for (band = 0; band < 3; band++) {
			int mem_size = src_width[band];
			pDst = (unsigned char *) buffer[band][0];
			pSrc = (unsigned char *) rawData[band] + counter * buf_height[band] * src_width[band];

			for (i = 0; i < buf_height[band]; i++) {
				memcpy(pDst, pSrc, mem_size);
				pSrc += src_width[band];
				pDst += buf_width[band];
			}
		}
		jpeg_write_raw_data(&cinfo, buffer, max_line);
	}

	jpeg_finish_compress(&cinfo);
	fclose(outfile);
	jpeg_destroy_compress(&cinfo);

	return 0;
}

static int yuv422to420(unsigned char yuv422[], unsigned char yuv420[], int width, int height)
{
	int ynum = width*height;
	int i, j, k = 0;

	for (i = 0; i < ynum; i++)
		yuv420[i] = yuv422[i * 2];

	for (i = 0; i < height; i++) {
		if ((i % 2) != 0)
			continue;
		for (j = 0; j < (width / 2); j++) {
			if ((4 * j + 1) > (2 * width))
				break;
			yuv420[ynum + k * 2 * width / 4 + j] =
				yuv422[i * 2 * width + 4 * j + 1];
		}
		k++;
	}

	k = 0;
	for (i = 0; i < height; i++) {
		if ((i % 2) == 0)
			continue;
		for (j = 0; j < (width / 2); j++) {
			if ((4 * j + 3) > (2 * width))
				break;
			yuv420[ynum + ynum / 4 + k * 2 * width / 4 + j] =
						 yuv422[i * 2 * width + 4 * j + 3];
		}
		k++;
	}

	return 1;
}

static void store_yuv420_and_jpeg(const void *p, int size)
{
	FILE *fp = fopen(CAPTURE_FILE_YUV, "wb");

	yuv420buf = calloc((unsigned int)size, sizeof(unsigned char));
	yuv422to420((char *)p, yuv420buf, 640, 480);
	if (fp < 0) {
	    printf("open frame data file failed\n");
	    return;
	}
	fwrite(yuv420buf, 1, size, fp);
	fclose(fp);

	write_JPEG_file(CAPTURE_FILE_JPG, yuv420buf, 100, 640, 480);

	return;
}

static void store_yuyv(const void *p, int size)
{
	FILE *fp = fopen(CAPTURE_FILE_YUYV, "wb");
	if (fp < 0) {
		printf("open frame data file failed\n");
		return;
	}
	fwrite(p, 1, size, fp);
	fclose(fp);

	return;
}

static void process_image(const void *p, int size)
{
	store_yuyv(p, size);
	store_yuv420_and_jpeg(p, size);
}

static int read_frame(void)
{
	struct v4l2_buffer buf;
	unsigned int i;

	switch (io) {
	case IO_METHOD_READ:
		if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				errno_exit("read");
			}
		}
		process_image(buffers[0].start, buffers[0].length);
		break;
	case IO_METHOD_MMAP:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}
		assert(buf.index < n_buffers);
		process_image(buffers[buf.index].start, buf.length);
		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
		break;
	case IO_METHOD_USERPTR:
		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}
		for (i = 0; i < n_buffers; ++i)
			if (buf.m.userptr == (unsigned long) buffers[i].start
					&& buf.length == buffers[i].length)
				break;

			assert(i < n_buffers);
			process_image((void *) buf.m.userptr, buf.length);
			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
			break;
		}

	return 1;
}

static void mainloop(void)
{
	unsigned int count;
	count = 100;

	while (count-- > 0) {
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO(&fds);
			FD_SET(fd, &fds);

			/* Timeout. */
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			r = select(fd + 1, &fds, NULL, NULL, &tv);

			if (-1 == r) {
				if (EINTR == errno)
					continue;

				errno_exit("select");
			}

			if (0 == r) {
				fprintf(stderr, "select timeout\n");
				exit(EXIT_FAILURE);
			}

			if (read_frame())
				return;

			/* EAGAIN - continue select loop. */
		}
	}
}

static void stop_capturing(void)
{
	enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type)) {
			errno_exit("VIDIOC_STREAMOFF");
		}
		break;
	}
}

#define PAGE_SHIFT  12
#define CAM_PAGE_MASK ((0x1<<PAGE_SHIFT) - 1)
#define CAPBUF_NUM 3


#define _TGT_AP_CAM_MEM_SIZE		4
#define _TGT_AP_OS_MEM_SIZE		236
#define _TGT_AP_GFX_MEM_SIZE		0
#define _TGT_AP_GFX_MEM_BASE	(_TGT_AP_OS_MEM_SIZE)
#define _TGT_AP_CAM_MEM_BASE	(_TGT_AP_GFX_MEM_BASE + _TGT_AP_GFX_MEM_SIZE)

typedef struct CAPBUF_t {
	unsigned long virt_addr;
	unsigned long phys_addr;
	unsigned long length;
} CAPBUF_t;

static void *mCamReserveBase;

static void v4l2QueryBufWithUserPtr(void)
{
	int mCapFrameSize;
	long unsigned int  mFrameBufferSize;
	unsigned long mCapBufCnt;
	unsigned long mOutBufCnt;
	int mDevMemFd;
	CAPBUF_t mCaptureBuf[CAPBUF_NUM];
	struct v4l2_buffer buf;


	unsigned long long *pBuf = NULL;
	unsigned long i, j;
	unsigned long va, pa, offs;
	void *map_base;

	mDevMemFd = open("/dev/mem", O_RDWR);
	if (mDevMemFd == -1)
		printf("open /dev/mem failed!");

	mCapFrameSize = (640 * 480 * 2);
	mFrameBufferSize = (640 * 480 * 2);
	mCapBufCnt = 1;
	mOutBufCnt = 1;

	unsigned long cap_size = (mCapFrameSize & CAM_PAGE_MASK) ?
							((mCapFrameSize + CAM_PAGE_MASK) & ~CAM_PAGE_MASK) :
							mCapFrameSize;
	unsigned long out_size = (mFrameBufferSize & CAM_PAGE_MASK) ?
							((mFrameBufferSize + CAM_PAGE_MASK) & ~CAM_PAGE_MASK) :
							mFrameBufferSize;
	unsigned long contig_size = cap_size * mCapBufCnt + out_size * mOutBufCnt;
	unsigned long base = _TGT_AP_CAM_MEM_BASE;

	pa = 0x80000000 + (base << 20);
	offs = pa & CAM_PAGE_MASK;
	mCamReserveBase = mmap(NULL,
							_TGT_AP_CAM_MEM_SIZE << 20,
							PROT_READ | PROT_WRITE,
							MAP_SHARED,
							mDevMemFd,
							pa & ~CAM_PAGE_MASK);
	if (mCamReserveBase == MAP_FAILED)
		printf("%s: Failed to mmap buffer: %lu", __FUNCTION__, pa);

	va = (unsigned long)mCamReserveBase + offs;

	/* allocate buffer for capture */
	for (i = 0; i < mCapBufCnt; i++) {
		memset(&mCaptureBuf[i], 0, sizeof(CAPBUF_t));
		mCaptureBuf[i].virt_addr = va + i * cap_size;
		mCaptureBuf[i].phys_addr = pa + i * cap_size;
		mCaptureBuf[i].length = mCapFrameSize;

		CLEAR(buf);
		buf.m.userptr = mCaptureBuf[i].virt_addr;
		buf.length = mCaptureBuf[i].length;

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
		buf.index = i;

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
	}
}

static void start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;
	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");

		break;
	case IO_METHOD_USERPTR:
#if 1
		v4l2QueryBufWithUserPtr();
#else
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long) buffers[i].start;
			buf.length = buffers[i].length;

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
#endif
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");

		break;
	}
}

static void uninit_device(void)
{
	unsigned int i;

	switch (io) {
	case IO_METHOD_READ:
		free(buffers[0].start);
		break;
	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i)
			if (-1 == munmap(buffers[i].start, buffers[i].length))
				errno_exit("munmap");
		break;
	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i)
			free(buffers[i].start);
		break;
	}

	free(buffers);
}

static void init_read(unsigned int buffer_size)
{
	buffers = calloc(1, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);

	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
}

static void init_mmap(void)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support"
					"memory mapping\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
		exit(EXIT_FAILURE);
	}

	buffers = calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL /* start anywhere */, buf.length,
		PROT_READ | PROT_WRITE /* required */,
		MAP_SHARED /* recommended */, fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}

static void init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size;

	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
					"user pointer i/o\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	buffers = calloc(4, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = memalign(/* boundary */page_size,
		buffer_size);

		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

static void init_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s does not support read i/o\n", dev_name);
			exit(EXIT_FAILURE);
		}
	break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s does not support streaming i/o\n", dev_name);
			exit(EXIT_FAILURE);
		}

		break;
	}

	/* Select video input, video standard and tune here. */
	CLEAR(cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {
		/* Errors ignored. */
	}

	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 640;
	fmt.fmt.pix.height = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	/* if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) */
	if (-1 == xioctl(fd, VIDIOC_TRY_FMT, &fmt))
		errno_exit("VIDIOC_S_FMT");

	/* Note VIDIOC_S_FMT may change width and height. */

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	switch (io) {
	case IO_METHOD_READ:
		init_read(fmt.fmt.pix.sizeimage);
		break;
	case IO_METHOD_MMAP:
		init_mmap();
		break;
	case IO_METHOD_USERPTR:
		init_userp(fmt.fmt.pix.sizeimage);
		break;
	}
}

static void close_device(void)
{
	if (-1 == close(fd))
		errno_exit("close");

	fd = -1;
}

/* dev_name = "/dev/video0" */
static void open_device(void)
{
	struct stat st;

	if (-1 == stat(dev_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno,
		strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	fd = open(dev_name, O_RDWR /* required */| O_NONBLOCK, 0);

	if (-1 == fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n", dev_name, errno,
		strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static void usage(FILE *fp, int argc, char **argv)
{
	fprintf(fp,
			"Usage: %s [options]\n\n"
			"Options:\n"
			"-d | --device name   Video device name [/dev/video]\n"
			"-h | --help          Print this message\n"
			"-m | --mmap          Use memory mapped buffers(recommend)\n"
			"-r | --read          Use read() calls\n"
			"-u | --userp         Use application allocated buffers(for Android)\n"
			"", argv[0]);
}

static const char short_options[] = "d:hmru";

static const struct option long_options[] = {
		{"device", required_argument, NULL, 'd'},
		{ "help", no_argument, NULL, 'h' },
		{ "mmap", no_argument, NULL, 'm' },
		{ "read", no_argument, NULL, 'r' },
		{ "userp", no_argument, NULL, 'u' },
		{ 0, 0, 0, 0 },
		};

int main(int argc, char **argv)
{
	dev_name = "/dev/video0";

	for (;;) {
		int index;
		int c;

		c = getopt_long(argc, argv, short_options, long_options, &index);

		if (-1 == c)
			break;

		switch (c) {
		case 0: /* getopt_long() flag */
			break;
		case 'd':
			dev_name = optarg;
			break;
		case 'h':
			usage(stdout, argc, argv);
			exit(EXIT_SUCCESS);
		case 'm':
			io = IO_METHOD_MMAP;
			break;
		case 'r':
			io = IO_METHOD_READ;
			break;
		case 'u':
			io = IO_METHOD_USERPTR;
			break;
		default:
			usage(stderr, argc, argv);
			exit(EXIT_FAILURE);
		}
	}

	open_device();

	init_device();

	start_capturing();

	mainloop();

	stop_capturing();

	uninit_device();

	close_device();

	exit(EXIT_SUCCESS);

	munmap(mCamReserveBase, (_TGT_AP_CAM_MEM_SIZE << 20));

	return 0;
}

#include "wavinfo.h"

#include <sys/time.h>
double dtime() {
	struct timeval ct;
	struct timezone tz;
	gettimeofday(&ct, &tz);
	return (ct.tv_sec + ct.tv_usec/1e6);
}

pthread_mutex_t m_ima = PTHREAD_MUTEX_INITIALIZER, m_wvlt = PTHREAD_MUTEX_INITIALIZER,
	m_hist = PTHREAD_MUTEX_INITIALIZER;
unsigned int bufsize;

void help(char *s){
	fprintf(stderr, "\n\n%s, simple interface for LOMO's webcam \"microscope\"\n", s);
	fprintf(stderr, "Usage: %s [-h] [-d videodev] [-c channel]\n\n", s);
	fprintf(stderr,
		"-h, --help:\tthis help\n"
		"-d, --device:\tcapture from <videodev>\n"
		"-c, --channel:\tset channel <channel> (0..3)\n"
	);
	fprintf(stderr, "\n\n");
}

JOCTET *jpeg_buf = NULL;

void *process_grabbing(){
	while(1){
		jpeg_buf = grab_next();
	}
}

void* wvlt_process(){
	int cntr = -1;
	fillSinBuf();
	while(1){
		if(pthread_mutex_trylock(&m_ima) != 0) continue;
		if(!jpeg_buf){
			pthread_mutex_unlock(&m_ima);
			continue;
		}
		BitmapBits = uncompressImage(jpeg_buf);
		pthread_mutex_unlock(&m_ima);
		cntr++;
		if(cntr == 0){
			initWavelet(0);
			continue;
		}
		if(cntr<3){
			initWavelet(1);
			continue;
		}
		cntr = -1;
		if(WaveWindow != -1 || SubWindow != -1){
			pthread_mutex_lock(&m_wvlt);
			mkWavedec();
			pthread_mutex_unlock(&m_wvlt);
		}
		if(SubWindow != -1){
			pthread_mutex_lock(&m_hist);
			buildHistogram();
			pthread_mutex_unlock(&m_hist);
		}
	}
}

void init_source(j_decompress_ptr cinfo){}
int fill_input_buffer(j_decompress_ptr cinfo){
	struct jpeg_source_mgr * src = cinfo->src;
	static JOCTET FakeEOI[] = { 0xFF, JPEG_EOI };
	WARNMS(cinfo, JWRN_JPEG_EOF);
	src->next_input_byte = FakeEOI;
	src->bytes_in_buffer = 2;
	return 1;
}
void skip_input_data(j_decompress_ptr cinfo, long num_bytes){
	struct jpeg_source_mgr * src = cinfo->src;
	if(num_bytes >= (long)src->bytes_in_buffer){
		fill_input_buffer(cinfo);
		return;
	}
	src->bytes_in_buffer -= num_bytes;
	src->next_input_byte += num_bytes;
}
void term_source(j_decompress_ptr cinfo){}
void jpeg_mem_src(j_decompress_ptr cinfo, JOCTET *pData, int FileSize){
	struct jpeg_source_mgr * src;
	if (cinfo->src == NULL){
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			sizeof(struct jpeg_source_mgr));
	}
	src = cinfo->src;
	src->init_source = init_source;
	src->fill_input_buffer = fill_input_buffer;
	src->skip_input_data = skip_input_data;
	src->resync_to_restart = jpeg_resync_to_restart;
	src->term_source = term_source;
	src->bytes_in_buffer = FileSize;
	src->next_input_byte = pData;
}

GLubyte *uncompressImage(JOCTET *inbuf){
	static struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	static GLubyte *imPtr = NULL;
	static int bufferSize;
	int readlines, linesize;
	GLubyte *ptr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, inbuf, bufsize); 
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
	if(!imPtr){
		bufferSize = GRAB_WIDTH * GRAB_HEIGHT * cinfo.num_components;
		imPtr = (GLubyte *)malloc(bufferSize);
		WARN("init imPtr, size=%d\n", bufferSize);
	}
	linesize = cinfo.num_components * cinfo.output_width;
//	ptr = imPtr + (bufferSize - linesize);
	ptr = imPtr;
	while(cinfo.output_scanline < cinfo.output_height){
		readlines = jpeg_read_scanlines(&cinfo, &ptr, 1);
		//ptr -= readlines * linesize;
		ptr += readlines * linesize;
	}
	jpeg_finish_decompress(&cinfo); 
	return imPtr;
}

int main(int argc, char** argv){
	char *grabdev = NULL;
	int c, grabchan = 0;
	while((c = getopt_long(argc, argv, "hd:c:", long_option, NULL)) != -1){
		switch(c){
		case 'h':
			help(argv[0]);
			break;
		case 'd':
            grabdev = strdup(optarg);
            break;
        case 'c':
        	grabchan = atoi(optarg);
        	if(grabchan < 0) grabchan = 0;
        	else if(grabchan > 3) grabchan = 3;
        	break;
		}
	}
	bufsize = grab_init(grabdev, grabchan);
	pthread_create(&thread, NULL, process_grabbing, NULL);
	pthread_create(&thread_m, NULL, wvlt_process, NULL);
	BMPview();
	return (0);
}


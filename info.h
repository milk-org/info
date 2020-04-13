#if !defined(INFO_H)
#define INFO_H


void __attribute__((constructor)) libinit_info();


/*
int kbdhit(void);

int print_header(const char *str, char c);

struct timespec info_time_diff(struct timespec start, struct timespec end);


long brighter(
    const char *ID_name,
    double      value
);


errno_t img_nbpix_flux(
    const char *ID_name
);

float img_percentile_float(
    const char *ID_name,
    float       p
);

double img_percentile_double(
    const char *ID_name,
    double      p
);

double img_percentile(
    const char *ID_name,
    double      p
);


int img_histoc(const char *ID_name, const char *fname);

errno_t make_histogram(
    const char *ID_name,
    const char *ID_out_name,
    double      min,
    double      max,
    long        nbsteps
);


double ssquare(const char *ID_name);

double rms_dev(const char *ID_name);

errno_t info_image_stats(
    const char *ID_name,
    const char *options
);

imageID info_cubestats(
    const char *ID_name,
    const char *IDmask_name,
    const char *outfname
);

double img_min(const char *ID_name);

double img_max(const char *ID_name);

errno_t profile(
    const char *ID_name,
    const char *outfile,
    double      xcenter,
    double      ycenter,
    double      step,
    long        nb_step
);



errno_t profile2im(
    const char     *profile_name,
    long            nbpoints,
    unsigned long   size,
    double          xcenter,
    double          ycenter,
    double          radius,
    const char     *out
);

errno_t printpix(
    const char *ID_name,
    const char *filename
);

double background_photon_noise(
    const char *ID_name
);

int test_structure_function(const char *ID_name, long NBpoints,
                            const char *fname);

imageID full_structure_function(
    const char *ID_name,
    long        NBpoints,
    const char *ID_out
);



imageID info_cubeMatchMatrix(
    const char *IDin_name,
    const char *IDout_name
);

*/

#endif

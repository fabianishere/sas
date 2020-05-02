#ifndef SAS_DRIVERS_OSS_H
#define SAS_DRIVERS_OSS_H

#include <rxc/rxc.h>

/**
 * Create an audio sink for the Linux OSS interface.
 *
 * @return The sink to write the audio samples to or <code>NULL</code> on
 * allocation failure.
 */
struct rxc_sink * sas_drivers_oss(void);

#endif /* SAS_DRIVERS_WAV_H */

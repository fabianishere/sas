#ifndef SAS_DRIVERS_IGNORE_H
#define SAS_DRIVERS_IGNORE_H

#include <rxc/rxc.h>

/**
 * Create an audio sink that ignores all incoming chunks.
 *
 * @return The sink to write the audio samples to or <code>NULL</code> on
 * allocation failure.
 */
struct rxc_sink * sas_drivers_ignore(void);

#endif /* SAS_DRIVERS_IGNORE_H */

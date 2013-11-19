
/**
 * VMMLib - Vector & Matrix Math Lib
 *
 * @author Stefan Eilemann <eilemann@gmail.com>
 * @license revised BSD license, check LICENSE
 */

#ifndef VMMLIB_VERSION_H
#define VMMLIB_VERSION_H

namespace vmml
{
    // vmmlib version macros and functions
    /** The current major version. @version 1.5 */
#   define VMMLIB_VERSION_MAJOR @VERSION_MAJOR@

    /** The current minor version. @version 1.5 */
#   define VMMLIB_VERSION_MINOR @VERSION_MINOR@

    /** The current patch level. @version 1.5 */
#   define VMMLIB_VERSION_PATCH @VERSION_PATCH@

/** True if the current version is newer than the given one. @version 1.5 */
#   define VMMLIB_VERSION_GT( MAJOR, MINOR, PATCH )                   \
    ( (VMMLIB_VERSION_MAJOR>MAJOR) ||                                 \
      (VMMLIB_VERSION_MAJOR==MAJOR &&                                 \
       (VMMLIB_VERSION_MINOR>MINOR ||                                 \
        (VMMLIB_VERSION_MINOR==MINOR && VMMLIB_VERSION_PATCH>PATCH))))

/** True if the current version is equal or newer to the given. @version 1.5 */
#   define VMMLIB_VERSION_GE( MAJOR, MINOR, PATCH )                       \
    ( (VMMLIB_VERSION_MAJOR>MAJOR) ||                                     \
      (VMMLIB_VERSION_MAJOR==MAJOR &&                                     \
       (VMMLIB_VERSION_MINOR>MINOR ||                                     \
        (VMMLIB_VERSION_MINOR==MINOR && VMMLIB_VERSION_PATCH>=PATCH))))

/** True if the current version is older than the given one. @version 1.5 */
#   define VMMLIB_VERSION_LT( MAJOR, MINOR, PATCH )                       \
    ( (VMMLIB_VERSION_MAJOR<MAJOR) ||                                     \
      (VMMLIB_VERSION_MAJOR==MAJOR &&                                     \
       (VMMLIB_VERSION_MINOR<MINOR ||                                     \
        (VMMLIB_VERSION_MINOR==MINOR && VMMLIB_VERSION_PATCH<PATCH))))

/** True if the current version is older or equal to the given. @version 1.5 */
#   define VMMLIB_VERSION_LE( MAJOR, MINOR, PATCH )                       \
    ( (VMMLIB_VERSION_MAJOR<MAJOR) ||                                     \
      (VMMLIB_VERSION_MAJOR==MAJOR &&                                     \
       (VMMLIB_VERSION_MINOR<MINOR ||                                     \
        (VMMLIB_VERSION_MINOR==MINOR && VMMLIB_VERSION_PATCH<=PATCH))))
}

#endif //VMMLIB_VERSION_H

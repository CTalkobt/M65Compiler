/* tgi/tgi-error.h — TGI error codes (as cc65's)
 *
 * tgi_geterror returns the last one (and clears it), tgi_geterrormsg
 * describes one.
 */

#pragma once

#define TGI_ERR_OK              0       /* No error */
#define TGI_ERR_NO_DRIVER       1       /* No driver available */
#define TGI_ERR_CANNOT_LOAD     2       /* Error loading driver or font */
#define TGI_ERR_INV_DRIVER      3       /* Invalid driver */
#define TGI_ERR_INV_MODE        4       /* Mode not supported by driver */
#define TGI_ERR_INV_ARG         5       /* Invalid function argument */
#define TGI_ERR_INV_FUNC        6       /* Function not supported */
#define TGI_ERR_INV_FONT        7       /* Font file is invalid */
#define TGI_ERR_NO_RES          8       /* Out of resources */
#define TGI_ERR_INSTALLED       9       /* A driver is already installed */

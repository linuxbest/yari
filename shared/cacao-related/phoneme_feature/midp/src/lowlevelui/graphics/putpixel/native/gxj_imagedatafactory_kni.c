/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

#include <stddef.h>
#include <string.h>

#include <sni.h>
#include <commonKNIMacros.h>
#include <midpUtilKni.h>
#include <midpMalloc.h>

#include <gx_image.h>
#include <gxutl_image.h>
#include <gxutl_graphics.h>

#include <gxj_putpixel.h>
#include "gxj_intern_graphics.h"
#include "gxj_intern_image.h"
#include "gxj_intern_putpixel.h"

#if ENABLE_IMAGE_CACHE
#include <imageCache.h>
#endif

/** Convenenient for convert Java image object to screen buffer */
#define getImageScreenBuffer(jimgData,sbuf) \
	gxj_get_image_screen_buffer_impl(GXAPI_GET_IMAGEDATA_PTR(jimgData), \
				 sbuf, NULL)

/**
 * Create native representation for a image.
 *
 * @param jimg Java Image ROM structure to convert from
 * @param sbuf pointer to Screen buffer structure to populate
 * @param g optional Graphics object for debugging clip code.
 *	    give NULL if don't care.
 *
 * @return the given 'sbuf' pointer for convenient usage,
 *	   or NULL if the image is null.
 */
gxj_screen_buffer* gxj_get_image_screen_buffer_impl(const java_imagedata *img,
						    gxj_screen_buffer *sbuf,
						    jobject graphics) {

    /* NOTE:
     * Since this routine is called by every graphics operations
     * We use ROMStruct directly instead of macros
     * like JavaByteArray, etc, for max performance.
     */
    if (img == NULL) {
	return NULL;
    }

    sbuf->width  = img->width;
    sbuf->height = img->height;

    /* Only use nativePixelData and nativeAlphaData if
     * pixelData is null */
    if (img->pixelData != NULL) {
	sbuf->pixelData = (gxj_pixel_type *)&(img->pixelData->elements[0]);
	sbuf->alphaData = (img->alphaData != NULL)
			    ? (gxj_alpha_type *)&(img->alphaData->elements[0])
			    : NULL;
    } else {
	sbuf->pixelData = (gxj_pixel_type *)img->nativePixelData;
	sbuf->alphaData = (gxj_alpha_type *)img->nativeAlphaData;
    }

#if ENABLE_BOUNDS_CHECKS
    sbuf->g = (graphics != NULL) ? GXAPI_GET_GRAPHICS_PTR(graphics) : NULL;
#else
    (void)graphics; /* Surpress unused parameter warning */
#endif

    return sbuf;
}

/**
 * Decodes the given input data into a cache representation that can
 * be saved and loaded quickly.
 * The input data should be in a self-identifying format; that is,
 * the data must contain a description of the decoding process.
 *
 *  @param srcBuffer input data to be decoded.
 *  @param length length of the input data.
 *  @param ret_dataBuffer pointer to the platform representation data that
 *         be saved.
 *  @param ret_length pointer to the length of the return data.
 *  @return one of error codes:
 *              MIDP_ERROR_NONE,
 *              MIDP_ERROR_OUT_MEM,
 *              MIDP_ERROR_UNSUPPORTED,
 *              MIDP_ERROR_OUT_OF_RESOURCE,
 *              MIDP_ERROR_IMAGE_CORRUPTED
 */
MIDP_ERROR gx_decode_data2cache(unsigned char* srcBuffer,
				   unsigned int length,
				   unsigned char** ret_dataBuffer,
				   unsigned int* ret_length) {

    unsigned int pixelSize, alphaSize;
    gxutl_image_format format;
    MIDP_ERROR err;
    gxj_screen_buffer sbuf;
    gxutl_image_buffer_raw *rawBuffer;
    gxutl_native_image_error_codes creationError = GXUTL_NATIVE_IMAGE_NO_ERROR;

    err = gxutl_image_get_info(srcBuffer, length,
			    &format, (unsigned int *)&sbuf.width,
			    (unsigned int *)&sbuf.height);
    if (err != MIDP_ERROR_NONE) {
	return err;
    }

    pixelSize = sizeof(gxj_pixel_type) * sbuf.width * sbuf.height;
    alphaSize = sizeof(gxj_alpha_type) * sbuf.width * sbuf.height;

    switch (format) {

    case GXUTL_IMAGE_FORMAT_JPEG:
	/* JPEG does not contain alpha data */
	alphaSize = 0;
	/* Fall through */

    case GXUTL_IMAGE_FORMAT_PNG:
	/* Decode PNG/JPEG to screen buffer format */
	rawBuffer = (gxutl_image_buffer_raw *)
	  midpMalloc(offsetof(gxutl_image_buffer_raw, data)+pixelSize+alphaSize);

	if (rawBuffer == NULL) {
	    return MIDP_ERROR_OUT_MEM;
	}

	sbuf.pixelData = (gxj_pixel_type *)rawBuffer->data;

	if (format == GXUTL_IMAGE_FORMAT_PNG) {
	    sbuf.alphaData = rawBuffer->data + pixelSize;

	    rawBuffer->hasAlpha = decode_png(srcBuffer, length,
					     &sbuf, &creationError);
	    if (!rawBuffer->hasAlpha) {
		sbuf.alphaData = NULL;
		alphaSize = 0; /* Exclude alpha data */
	    }
	} else {
	    sbuf.alphaData = NULL;

	    rawBuffer->hasAlpha = KNI_FALSE;

	    decode_jpeg(srcBuffer, length, &sbuf, &creationError);
	}

	if (GXUTL_NATIVE_IMAGE_NO_ERROR != creationError) {
	    midpFree(rawBuffer);
	    return MIDP_ERROR_IMAGE_CORRUPTED;
	}

	memcpy(rawBuffer->header, gxutl_raw_header, 4);
	rawBuffer->width  = sbuf.width;		/* Use default endian */
	rawBuffer->height = sbuf.height;	/* Use default endian */

	*ret_dataBuffer = (unsigned char *)rawBuffer;
	*ret_length = offsetof(gxutl_image_buffer_raw, data)+pixelSize+alphaSize;

	return MIDP_ERROR_NONE;

    case GXUTL_IMAGE_FORMAT_RAW:
	/* Already in screen buffer format, simply copy the data */
	*ret_dataBuffer = (unsigned char *)midpMalloc(length);
	if (*ret_dataBuffer == NULL) {
	    return MIDP_ERROR_OUT_MEM;
	} else {
	    memcpy(*ret_dataBuffer, srcBuffer, length);
	    *ret_length = length;
	    return MIDP_ERROR_NONE;
	}

    default:
	return MIDP_ERROR_UNSUPPORTED;
    } /* switch (image_type) */
}

/**
 * Get pointer to internal buffer of Java byte array and
 * check that expected offset/length can be applied to the buffer
 *
 * @param byteArray Java byte array object to get buffer from
 * @param offset offset of the data needed in the buffer
 * @param length length of the data needed in the buffer
 *          starting from the offset
 * @return pointer to the buffer, or NULL if offset/length are
 *    are not applicable.
 */
static unsigned char *gx_get_java_byte_buffer(KNIDECLARGS
    jobject byteArray, int offset, int length) {

    unsigned char *buffer = (unsigned char *)JavaByteArray(byteArray);
    int byteArrayLength = KNI_GetArrayLength(byteArray);
    if (offset < 0 || length < 0 || offset + length > byteArrayLength ) {
        KNI_ThrowNew(midpArrayIndexOutOfBoundsException, NULL);
        return NULL;
    }
    return buffer;
}

/**
 * Load Java ImageData instance with image data in RAW format.
 * Image data is provided either in native buffer, or in Java
 * byte array. Java array is used with more priority.
 *
 * @param imageData Java ImageData object to be loaded with image data
 * @param nativeBuffer pointer to native buffer with raw image data,
 *          this parameter is alternative to javaBuffer
 * @param javaBuffer Java byte array with raw image data,
 *          this parameter is alternative to nativeBuffer
 * @param offset offset of the raw image data in the buffer
 * @param length length of the raw image data in the buffer
 *          starting from the offset
 *
 * @return KNI_TRUE in the case ImageData is successfully loaded with
 *    raw image data, otherwise KNI_FALSE.
 */
static int gx_load_imagedata_from_raw_buffer(KNIDECLARGS jobject imageData,
    unsigned char *nativeBuffer, jobject javaBuffer,
    int offset, int length) {

    int imageSize;
    int pixelSize, alphaSize;
    int status = KNI_FALSE;
    gxutl_image_buffer_raw *rawBuffer = NULL;

    KNI_StartHandles(2);
    KNI_DeclareHandle(pixelData);
    KNI_DeclareHandle(alphaData);

    do {
        /** Check native and Java buffer parameters */
        if (!KNI_IsNullHandle(javaBuffer)) {
            if (nativeBuffer != NULL) {
                REPORT_ERROR(LC_LOWUI,
                    "Native and Java buffers should not be used together");
                break;
            }
            nativeBuffer = gx_get_java_byte_buffer(KNIPASSARGS
                javaBuffer, offset, length);
        }
        if (nativeBuffer == NULL) {
            REPORT_ERROR(LC_LOWUI,
                "Null raw image buffer is provided");
            break;
        }

        /** Check header */
        rawBuffer = (gxutl_image_buffer_raw *)(nativeBuffer + offset);
        if (memcmp(rawBuffer->header, gxutl_raw_header, 4) != 0) {
            REPORT_ERROR(LC_LOWUI, "Unexpected raw image type");
            break;
        }

        imageSize = rawBuffer->width * rawBuffer->height;
        pixelSize = sizeof(gxj_pixel_type) * imageSize;
        alphaSize = 0;
        if (rawBuffer->hasAlpha) {
            alphaSize = sizeof(gxj_alpha_type) * imageSize;
        }

        /** Check data array length */
        if ((unsigned int)length !=
            (offsetof(gxutl_image_buffer_raw, data)
                + pixelSize + alphaSize)) {
            REPORT_ERROR(LC_LOWUI, "Raw image is corrupted");
            break;
        }

        if (rawBuffer->hasAlpha) {
            /* Has alpha */
            SNI_NewArray(SNI_BYTE_ARRAY, alphaSize, alphaData);
            if (KNI_IsNullHandle(alphaData)) {
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
                break;
            }
            /** Link the new array into ImageData to protect if from GC */
            midp_set_jobject_field(KNIPASSARGS imageData, "alphaData", "[B", alphaData);

            /** New array allocation could cause GC and buffer moving */
            if (!KNI_IsNullHandle(javaBuffer)) {
                nativeBuffer = gx_get_java_byte_buffer(KNIPASSARGS
                    javaBuffer, offset, length);
                rawBuffer = (gxutl_image_buffer_raw *)
                    (nativeBuffer + offset);
            }
            memcpy(JavaByteArray(alphaData),
                rawBuffer->data + pixelSize, alphaSize);
        }

        SNI_NewArray(SNI_BYTE_ARRAY, pixelSize, pixelData);
        if (KNI_IsNullHandle(pixelData)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }
	    midp_set_jobject_field(KNIPASSARGS imageData, "pixelData", "[B", pixelData);

        /** New array allocation could cause GC and buffer moving */
        if (!KNI_IsNullHandle(javaBuffer)) {
            nativeBuffer = gx_get_java_byte_buffer(KNIPASSARGS
                javaBuffer, offset, length);
            rawBuffer = (gxutl_image_buffer_raw *)
                (nativeBuffer + offset);
        }
	    memcpy(JavaByteArray(pixelData), rawBuffer->data, pixelSize);

        GXAPI_GET_IMAGEDATA_PTR(imageData)->width =
            (jint)rawBuffer->width;
        GXAPI_GET_IMAGEDATA_PTR(imageData)->height =
            (jint)rawBuffer->height;
        status = KNI_TRUE;

    } while(0);

    KNI_EndHandles();
    return status;
}

/**
 * Loads a native image data from image cache into ImageData..
 * <p>
 * Java declaration:
 * <pre>
 *     boolean loadCachedImage0(ImageData imageData,
 *                              String suiteId, String resName);
 * </pre>
 *
 * @param imageData The ImageData to be populated
 * @param suiteId   The suite Id
 * @param resName   The name of the image resource
 * @return true if a cached image was loaded, false otherwise
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadCachedImage0) {
#if ENABLE_IMAGE_CACHE
    int len;
    SuiteIdType suiteId;
    jboolean status = KNI_FALSE;
    unsigned char *rawBuffer = NULL;

    KNI_StartHandles(3);

    /* A handle for which KNI_IsNullHandle() check is true */
    KNI_DeclareHandle(nullHandle);

    GET_PARAMETER_AS_PCSL_STRING(3, resName)

    KNI_DeclareHandle(imageData);
    KNI_GetParameterAsObject(1, imageData);

    suiteId = KNI_GetParameterAsInt(2);

    len = loadImageFromCache(suiteId, &resName, &rawBuffer);
    if (len != -1 && rawBuffer != NULL) {
        /* image is found in cache */
        status = gx_load_imagedata_from_raw_buffer(KNIPASSARGS
            imageData, rawBuffer, nullHandle, 0, len);
    }

    midpFree(rawBuffer);

    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnBoolean(status);
#else
    KNI_ReturnBoolean(KNI_FALSE);
#endif
}

/**
 * Loads the <tt>ImageData</tt> with the given raw data array.
 * The array consists of raw image data including header info.
 * <p>
 * Java declaration:
 * <pre>
 *     loadRAW(Ljavax/microedition/lcdui/ImageData;[B)V
 * </pre>
 *
 * @param imageData The instance of ImageData to be loaded from array
 * @param imageBytes The array of raw image data
 * @param imageOffset the offset of the start of the data in the array
 * @param imageLength the length of the data in the array
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadRAW) {

    int offset = KNI_GetParameterAsInt(3);
    int length = KNI_GetParameterAsInt(4);

    KNI_StartHandles(2);
    KNI_DeclareHandle(imageData);
    KNI_DeclareHandle(imageBytes);

    KNI_GetParameterAsObject(1, imageData);
    KNI_GetParameterAsObject(2, imageBytes);

    /* Load ImageData content from Java byte array with raw image data */
    if (gx_load_imagedata_from_raw_buffer(KNIPASSARGS
            imageData, NULL, imageBytes, offset, length) != KNI_TRUE) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Decodes the given byte array into the <tt>ImageData</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     loadPNG(Ljavax/microedition/lcdui/ImageData;[BII)Z
 * </pre>
 *
 * @param imageData the ImageData to load to
 * @param imageBytes A byte array containing the encoded PNG image data
 * @param offset The start of the image data within the byte array
 * @param length The length of the image data in the byte array
 *
 * @return true if there is alpha data
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadPNG) {
    int            length = KNI_GetParameterAsInt(4);
    int            offset = KNI_GetParameterAsInt(3);
    int            status = KNI_TRUE;
    unsigned char* srcBuffer = NULL;
    gxj_screen_buffer            image;
    java_imagedata * midpImageData = NULL;

    /* variable to hold error codes */
    gxutl_native_image_error_codes creationError = GXUTL_NATIVE_IMAGE_NO_ERROR;

    KNI_StartHandles(4);
    KNI_DeclareHandle(alphaData);
    KNI_DeclareHandle(pixelData);
    KNI_DeclareHandle(pngData);
    KNI_DeclareHandle(imageData);

    KNI_GetParameterAsObject(2, pngData);
    KNI_GetParameterAsObject(1, imageData);

    midpImageData = GXAPI_GET_IMAGEDATA_PTR(imageData);

    /* assert
     * (KNI_IsNullHandle(pngData))
     */

    srcBuffer = (unsigned char *)JavaByteArray(pngData);
    /*
     * JAVA_TRACE("loadPNG pngData length=%d  %x\n",
     *            JavaByteArray(pngData)->length, srcBuffer);
     */

    image.width = midpImageData->width;
    image.height = midpImageData->height;

    unhand(jbyte_array, pixelData) = midpImageData->pixelData;
    if (!KNI_IsNullHandle(pixelData)) {
        image.pixelData = (gxj_pixel_type *)JavaByteArray(pixelData);
        /*
         * JAVA_TRACE("loadPNG pixelData length=%d\n",
         *            JavaByteArray(pixelData)->length);
         */
    } else {
	image.pixelData = NULL;
    }

    unhand(jbyte_array, alphaData) = midpImageData->alphaData;
    if (!KNI_IsNullHandle(alphaData)) {
        image.alphaData = (gxj_alpha_type *)JavaByteArray(alphaData);
        /*
         * JAVA_TRACE("decodePNG alphaData length=%d\n",
         *            JavaByteArray(alphaData)->length);
         */
    } else {
	image.alphaData = NULL;
    }

    /* assert
     * (imagedata.pixelData != NULL && imagedata.alphaData != NULL)
     */
    status = decode_png((srcBuffer + offset), length, &image, &creationError);

    if (GXUTL_NATIVE_IMAGE_NO_ERROR != creationError) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(status);
}

/**
 * Decodes the given byte array into the <tt>ImageData</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     loadJPG(Ljavax/microedition/lcdui/ImageData;[BII)V
 * </pre>
 *
 * @param imageData the ImageData to load to
 * @param imageBytes A byte array containing the encoded JPEG image data
 * @param imageOffset The start of the image data within the byte array
 * @param imageLength The length of the image data in the byte array
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadJPEG) {
    int            length = KNI_GetParameterAsInt(4);
    int            offset = KNI_GetParameterAsInt(3);
    unsigned char* srcBuffer = NULL;
    gxj_screen_buffer            image;
    java_imagedata * midpImageData = NULL;

    /* variable to hold error codes */
    gxutl_native_image_error_codes creationError = GXUTL_NATIVE_IMAGE_NO_ERROR;

    KNI_StartHandles(3);
    /* KNI_DeclareHandle(alphaData); */
    KNI_DeclareHandle(pixelData);
    KNI_DeclareHandle(jpegData);
    KNI_DeclareHandle(imageData);

    KNI_GetParameterAsObject(2, jpegData);
    KNI_GetParameterAsObject(1, imageData);

    midpImageData = GXAPI_GET_IMAGEDATA_PTR(imageData);

    /* assert
     * (KNI_IsNullHandle(jpegData))
     */

    srcBuffer = (unsigned char *)JavaByteArray(jpegData);
    /*
     * JAVA_TRACE("loadJPEG jpegData length=%d  %x\n",
     *            JavaByteArray(jpegData)->length, srcBuffer);
     */

    image.width = midpImageData->width;
    image.height = midpImageData->height;

    unhand(jbyte_array, pixelData) = midpImageData->pixelData;
    if (!KNI_IsNullHandle(pixelData)) {
        image.pixelData = (gxj_pixel_type *)JavaByteArray(pixelData);
        /*
         * JAVA_TRACE("loadJPEG pixelData length=%d\n",
         *            JavaByteArray(pixelData)->length);
         */
    }

    /* assert
     * (imagedata.pixelData != NULL)
     */
    decode_jpeg((srcBuffer + offset), length, &image, &creationError);

    if (GXUTL_NATIVE_IMAGE_NO_ERROR != creationError) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Initializes an <tt>ImageData</tt> from a romized pixel data
 * <p>
 * Java declaration:
 * <pre>
 *     loadRomizedImage(Ljavax/microedition/lcdui/ImageData;I)V
 * </pre>
 *
 * @param imageData The ImageData to load to
 * @param imageDataPtr native pointer to image data as Java int
 * @param imageDataLength length of image data array
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadRomizedImage) {
    int imageDataPtr = KNI_GetParameterAsInt(2);
    int imageDataLength = KNI_GetParameterAsInt(3);

    int alphaSize;
    int pixelSize;
    int imageSize;
    int expectedLength;

    gxutl_image_buffer_raw *rawBuffer;
    java_imagedata *midpImageData;

    jboolean status = KNI_FALSE;


    KNI_StartHandles(1);
    KNI_DeclareHandle(imageData);
    KNI_GetParameterAsObject(1, imageData);

    rawBuffer = (gxutl_image_buffer_raw*)imageDataPtr;

    do {
        if (rawBuffer == NULL) {
            REPORT_ERROR(LC_LOWUI, "Romized image data is null");

            status = KNI_FALSE;
            break;
        }

        /** Check header */
        if (memcmp(rawBuffer->header, gxutl_raw_header, 4) != 0) {
            REPORT_ERROR(LC_LOWUI, "Unexpected romized image type");

            status = KNI_FALSE;
            break;
        }

        imageSize = rawBuffer->width * rawBuffer->height;
        pixelSize = sizeof(gxj_pixel_type) * imageSize;
        alphaSize = 0;
        if (rawBuffer->hasAlpha) {
            alphaSize = sizeof(gxj_alpha_type) * imageSize;
        }

        /** Check data array length */
        expectedLength = offsetof(gxutl_image_buffer_raw, data) +
            pixelSize + alphaSize;
        if (imageDataLength != expectedLength) {
            REPORT_ERROR(LC_LOWUI,
                    "Unexpected romized image data array length");

            status = KNI_FALSE;
            break;
        }

        midpImageData = GXAPI_GET_IMAGEDATA_PTR(imageData);

        midpImageData->width = (jint)rawBuffer->width;
        midpImageData->height = (jint)rawBuffer->height;

        midpImageData->nativePixelData = (jint)rawBuffer->data;

        if (rawBuffer->hasAlpha) {
            midpImageData->nativeAlphaData =
                (jint)(rawBuffer->data + pixelSize);
        }

        status = KNI_TRUE;

    } while (0);

    KNI_EndHandles();
    KNI_ReturnBoolean(status);
}

/**
 * Loads the <tt>ImageData</tt> with the given ARGB integer
 * array. The array consists of values in the form of 0xAARRGGBB.
 * <p>
 * Java declaration:
 * <pre>
 *     loadRGB(Ljavax/microedition/lcdui/ImageData;[I)V
 * </pre>
 *
 * @param rgbData The array of argb image data
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadRGB) {
    /* jboolean processAlpha = KNI_GetParameterAsBoolean(2); */
    int height;
    int width;
    int *rgbBuffer;
    gxj_screen_buffer sbuf;

    KNI_StartHandles(2);
    KNI_DeclareHandle(rgbData);
    KNI_DeclareHandle(imageData);

    KNI_GetParameterAsObject(2, rgbData);
    KNI_GetParameterAsObject(1, imageData);

    width  = (int)GXAPI_GET_IMAGEDATA_PTR(imageData)->width;
    height = (int)GXAPI_GET_IMAGEDATA_PTR(imageData)->height;

    rgbBuffer = JavaIntArray(rgbData);
    if (getImageScreenBuffer(imageData, &sbuf) != NULL) {
        int i;
        int len = KNI_GetArrayLength(rgbData);
        int data_length = width * height;

        if (len > data_length) {
            len = data_length;
        }

        /* if (len != width*height) {
         *    JAVA_TRACE("len mismatch  %d !=  %d\n", len, width*height);
         * }
		 */

        if (sbuf.alphaData != NULL) {
            for (i = 0; i < len; i++) {
                sbuf.pixelData[i] = GXJ_RGB24TORGB16(rgbBuffer[i]);
                sbuf.alphaData[i] = (rgbBuffer[i] >> 24) & 0x00ff;
            }
        } else {
            for (i = 0; i < len; i++) {
                sbuf.pixelData[i] = GXJ_RGB24TORGB16(rgbBuffer[i]);
            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

#define PIXEL gxj_pixel_type
#define ALPHA gxj_alpha_type

void pixelCopy(PIXEL *src, const int srcLineW, const int srcXInc,
               const int srcYInc, const int srcXStart,
               PIXEL *dst, const int w, const int h) {
    int x, srcX;
    PIXEL *dstPtrEnd = dst + (h * w );

    (void)srcLineW; /* Surpress unused warning */

    for (; dst < dstPtrEnd; dst += w, src += srcYInc) {
        for (x = 0, srcX = srcXStart; x < w; srcX += srcXInc) {
            // printf("%d = %d\n", ((dst+x)-dstData), (src+srcX)-srcData);
            dst[x++] = src[srcX];
        }
    }
}

void pixelAndAlphaCopy(PIXEL *src, const int srcLineW, const int srcXInc,
                       const int srcYInc, const int srcXStart, PIXEL *dst,
                       const int w, const int h,
                       const ALPHA *srcAlpha, ALPHA *dstAlpha) {
    int x, srcX;
    PIXEL *dstPtrEnd = dst + (h * w );

    (void)srcLineW; /* Surpress unused warning */

    for (; dst < dstPtrEnd; dst += w, src += srcYInc,
        dstAlpha += w, srcAlpha += srcYInc) {
        for (x = 0, srcX = srcXStart; x < w; srcX += srcXInc) {
            // printf("%d = %d\n", ((dst+x)-dstData), (src+srcX)-srcData);
            dstAlpha[x] = srcAlpha[srcX];
            dst[x++] = src[srcX];
        }
    }
}

void blit(const gxj_screen_buffer *src, int xSrc, int ySrc, int width, int height,
          gxj_screen_buffer *dst, int transform) {
    PIXEL *srcPtr = NULL;
    int srcXInc=0, srcYInc=0, srcXStart=0;

    switch (transform) {
    case TRANS_NONE:
        srcPtr = (src->pixelData) + (ySrc * src->width + xSrc);
        srcYInc = src->width;
        srcXStart = 0;
        srcXInc = 1;
        break;
    case TRANS_MIRROR_ROT180:
        srcPtr = (src->pixelData) + ((ySrc + height - 1) * src->width + xSrc);
        srcYInc = -(src->width);
        srcXStart = 0;
        srcXInc = 1;
        break;
    case TRANS_MIRROR:
        srcPtr = (src->pixelData) + (ySrc * src->width + xSrc);
        srcYInc = src->width;
        srcXStart = width - 1;
        srcXInc = -1;
        break;
    case TRANS_ROT180:
        srcPtr = (src->pixelData) + ((ySrc + height - 1) * src->width + xSrc);
        srcYInc = -(src->width);
        srcXStart = width - 1;
        srcXInc = -1;
        break;
    case TRANS_MIRROR_ROT270:
        srcPtr = (src->pixelData) + (ySrc * src->width + xSrc);
        srcYInc = 1;
        srcXStart = 0;
        srcXInc = src->width;
        break;
    case TRANS_ROT90:
        srcPtr = (src->pixelData) + ((ySrc + height - 1) * src->width + xSrc);
        srcYInc = 1;
        srcXStart = 0;
        srcXInc = -(src->width);
        break;
    case TRANS_ROT270:
        srcPtr = (src->pixelData) + (ySrc * src->width + xSrc + width - 1);
        srcYInc = -1;
        srcXStart = 0;
        srcXInc = src->width;
        break;
    case TRANS_MIRROR_ROT90:
        srcPtr = (src->pixelData) + ((ySrc + height - 1) * src->width + xSrc);
        srcYInc = -1;
        srcXStart = width - 1;
        srcXInc = -(src->width);
        break;
    }

    if (transform & TRANSFORM_INVERTED_AXES) {
        if (src->alphaData == NULL) {
            pixelCopy(srcPtr, src->width, srcXInc, srcYInc, srcXStart,
                      dst->pixelData, height, width);
        } else {
            ALPHA *srcAlpha = src->alphaData + (srcPtr - src->pixelData);
            pixelAndAlphaCopy(srcPtr, src->width, srcXInc, srcYInc,
                              srcXStart,
                              dst->pixelData, height, width, srcAlpha,
                              dst->alphaData);
        }
    } else {
        if (src->alphaData == NULL) {
            pixelCopy(srcPtr, src->width, srcXInc, srcYInc, srcXStart,
                      dst->pixelData, width, height);
        } else {
            ALPHA *srcAlpha = src->alphaData + (srcPtr - src->pixelData);
            pixelAndAlphaCopy(srcPtr, src->width, srcXInc, srcYInc, srcXStart,
                              dst->pixelData, width, height,
                              srcAlpha, dst->alphaData);
        }
    }
}

/**
 * Copies the region of the specified <tt>ImageData</tt> to
 * the specified <tt>ImageData</tt> object.
 * <p>
 * Java declaration:
 * <pre>
 *     loadRegion(Ljavax/microedition/lcdui/ImageData;
 *                Ljavax/microedition/lcdui/ImageData;IIIII)V
 * </pre>
 *
 * @param dest the ImageData to copy to
 * @param source the source image to be copied from
 * @param x The x coordinate of the upper left corner of the
 *          region to copy
 * @param y The y coordinate of the upper left corner of the
 *          region to copy
 * @param width The width of the region to copy
 * @param height The height of the region to copy
 * @param transform The transform to apply to the selected region.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_ImageDataFactory_loadRegion) {
    int      transform = KNI_GetParameterAsInt(7);
    int         height = KNI_GetParameterAsInt(6);
    int          width = KNI_GetParameterAsInt(5);
    int              y = KNI_GetParameterAsInt(4);
    int              x = KNI_GetParameterAsInt(3);
    gxj_screen_buffer srcSBuf;
    gxj_screen_buffer dstSBuf;

    KNI_StartHandles(2);
    KNI_DeclareHandle(srcImg);
    KNI_DeclareHandle(destImg);

    KNI_GetParameterAsObject(2, srcImg);
    KNI_GetParameterAsObject(1, destImg);

    if (getImageScreenBuffer(destImg, &dstSBuf) != NULL &&
        getImageScreenBuffer(srcImg, &srcSBuf) != NULL) {

        blit(&srcSBuf, x, y, width, height, &dstSBuf, transform);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

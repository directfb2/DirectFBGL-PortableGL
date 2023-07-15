/*
   This file is part of DirectFB.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#ifndef __IDIRECTFBGL_PORTABLEGL_H__
#define __IDIRECTFBGL_PORTABLEGL_H__

#include <directfbgl.h>
#include <display/idirectfbsurface.h>
#define  PORTABLEGL_IMPLEMENTATION
#include "portablegl.h"

D_DEBUG_DOMAIN( DFBGL_PGL, "DirectFBGL/PGL", "DirectFBGL PortableGL Implementation" );

static DFBResult Probe    ( void             *ctx );

static DFBResult Construct( IDirectFBGL      *thiz,
                            IDirectFBSurface *surface,
                            IDirectFB        *idirectfb );

#include <direct/interface_implementation.h>

DIRECT_INTERFACE_IMPLEMENTATION( IDirectFBGL, PGL )

/**********************************************************************************************************************/

typedef struct {
     int               ref;        /* reference counter */

     glContext         pglContext;

     IDirectFBSurface *back;
     IDirectFBSurface *front;
} IDirectFBGL_PGL_data;

static DFBResult
pgl_flip_func( void *ctx )
{
     IDirectFBGL_PGL_data *data = ctx;

     data->front->Blit( data->front, data->back, NULL, 0, 0 );
     data->front->Flip( data->front, NULL, DSFLIP_WAITFORSYNC );

     return DFB_OK;
}

/**********************************************************************************************************************/

static void
IDirectFBGL_PGL_Destruct( IDirectFBGL *thiz )
{
     IDirectFBGL_PGL_data *data = thiz->priv;

     D_DEBUG_AT( DFBGL_PGL, "%s( %p )\n", __FUNCTION__, thiz );

     if (data->front)
          data->front->Release( data->front );

     if (data->back)
          data->back->Release( data->back );

     free_glContext( &data->pglContext );

     DIRECT_DEALLOCATE_INTERFACE( thiz );
}

static DirectResult
IDirectFBGL_PGL_AddRef( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL_PGL );

     D_DEBUG_AT( DFBGL_PGL, "%s( %p )\n", __FUNCTION__, thiz );

     data->ref++;

     return DFB_OK;
}

static DirectResult
IDirectFBGL_PGL_Release( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL_PGL )

     D_DEBUG_AT( DFBGL_PGL, "%s( %p )\n", __FUNCTION__, thiz );

     if (--data->ref == 0)
          IDirectFBGL_PGL_Destruct( thiz );

     return DFB_OK;
}

static DFBResult
IDirectFBGL_PGL_Lock( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL_PGL );

     D_DEBUG_AT( DFBGL_PGL, "%s( %p )\n", __FUNCTION__, thiz );

     set_glContext( &data->pglContext );

     return DFB_OK;
}

static DFBResult
IDirectFBGL_PGL_Unlock( IDirectFBGL *thiz )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL_PGL );

     D_DEBUG_AT( DFBGL_PGL, "%s( %p )\n", __FUNCTION__, thiz );

     set_glContext( NULL );

     return DFB_OK;
}

static DFBResult
IDirectFBGL_PGL_GetAttributes( IDirectFBGL     *thiz,
                               DFBGLAttributes *ret_attributes )
{
     DIRECT_INTERFACE_GET_DATA( IDirectFBGL_PGL );

     D_DEBUG_AT( DFBGL_PGL, "%s( %p )\n", __FUNCTION__, thiz );

     if (!ret_attributes)
          return DFB_INVARG;

     memset( ret_attributes, 0, sizeof(DFBGLAttributes) );

     ret_attributes->buffer_size  = 32;
     ret_attributes->depth_size   = 1;
     ret_attributes->red_size     = 8;
     ret_attributes->green_size   = 8;
     ret_attributes->blue_size    = 8;
     ret_attributes->alpha_size   = 8;

     ret_attributes->double_buffer = DFB_TRUE;

     return DFB_OK;
}

static DFBResult
IDirectFBGL_PGL_GetProcAddress( IDirectFBGL  *thiz,
                                const char   *name,
                                void        **ret_address )
{
     D_DEBUG_AT( DFBGL_PGL, "%s( %p )\n", __FUNCTION__, thiz );

     return DFB_FAILURE;
}

/**********************************************************************************************************************/

static DFBResult
Probe( void *ctx )
{
     return DFB_OK;
}

static DFBResult
Construct( IDirectFBGL      *thiz,
           IDirectFBSurface *surface,
           IDirectFB        *idirectfb )
{
     DFBResult              ret;
     int                    err;
     int                    width, height;
     DFBSurfacePixelFormat  pixelformat;
     DFBSurfaceDescription  desc;
     IDirectFBSurface_data *surface_data;
     u32                   *buf = NULL;

     DIRECT_ALLOCATE_INTERFACE_DATA( thiz, IDirectFBGL_PGL );

     D_DEBUG_AT( DFBGL_PGL, "%s( %p )\n", __FUNCTION__, thiz );

     data->ref = 1;

     surface->GetSize( surface, &width, &height );

     surface->GetPixelFormat( surface, &pixelformat );

     if (pixelformat == DSPF_RGB32) {
         int pitch;

         surface->Lock( surface, DSLF_WRITE, &buf, &pitch );
         surface->Unlock( surface );
     }

     err = init_glContext( &data->pglContext, &buf, width, height, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000 );
     if (!err) {
          D_ERROR( "DirectFBGL/PGL: Failed to initialize glContext!\n" );
          DIRECT_DEALLOCATE_INTERFACE( thiz );
          return DFB_FAILURE;
     }

     if (pixelformat != DSPF_RGB32) {
          desc.flags                 = DSDESC_PIXELFORMAT | DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PREALLOCATED;
          desc.pixelformat           = DSPF_RGB32;
          desc.width                 = width;
          desc.height                = height;
          desc.preallocated[0].data  = buf;
          desc.preallocated[0].pitch = width * 4;

          ret = idirectfb->CreateSurface( idirectfb, &desc, &data->back );
          if (ret) {
               D_DERROR( ret, "DirectFBGL/PGL: Failed to create back surface!\n" );
               goto error;
          }

          ret = surface->GetSubSurface( surface, NULL, &data->front );
          if (ret) {
               D_DERROR( ret, "DirectFBGL/PGL: Failed to create front surface!\n" );
               goto error;
          }

          surface_data = surface->priv;
          if (!surface_data)
               goto error;

          surface_data->flip_func     = pgl_flip_func;
          surface_data->flip_func_ctx = data;
     }

     thiz->AddRef         = IDirectFBGL_PGL_AddRef;
     thiz->Release        = IDirectFBGL_PGL_Release;
     thiz->Lock           = IDirectFBGL_PGL_Lock;
     thiz->Unlock         = IDirectFBGL_PGL_Unlock;
     thiz->GetAttributes  = IDirectFBGL_PGL_GetAttributes;
     thiz->GetProcAddress = IDirectFBGL_PGL_GetProcAddress;

     return DFB_OK;

error:
     if (data->front)
          data->front->Release( data->front );

     if (data->back)
          data->back->Release( data->back );

     DIRECT_DEALLOCATE_INTERFACE( thiz );

     return ret;
}

#endif

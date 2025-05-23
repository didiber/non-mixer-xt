/*******************************************************************************/
/* Copyright (C) 2008-2021 Jonathan Moore Liles (as "Non-Mixer")               */
/* Copyright (C) 2021- Stazed                                                  */
/*                                                                             */
/* This file is part of Non-Mixer-XT                                           */
/*                                                                             */
/* This program is free software; you can redistribute it and/or modify it     */
/* under the terms of the GNU General Public License as published by the       */
/* Free Software Foundation; either version 2 of the License, or (at your      */
/* option) any later version.                                                  */
/*                                                                             */
/* This program is distributed in the hope that it will be useful, but WITHOUT */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   */
/* more details.                                                               */
/*                                                                             */
/* You should have received a copy of the GNU General Public License along     */
/* with This program; see the file COPYING.  If not,write to the Free Software */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
/*******************************************************************************/

/*
 * File:   LADSPA_Plugin.H
 * Author: sspresto
 *
 * Created on November 24, 2022, 4:32 PM
 */

#pragma once

#ifdef LADSPA_SUPPORT

#include "../Plugin_Module.H"


class LADSPA_Plugin : public Plugin_Module
{

public:
    struct ImplementationData;

    ImplementationData *_idata;

    virtual bool load_plugin ( Module::Picked picked ) override;
    void create_audio_ports();
    void create_control_ports();

    virtual bool get_impulse_response ( sample_t *buf, nframes_t nframes ) override;

    bool configure_inputs ( int ) override;
    void handle_port_connection_change ( void ) override;
    void resize_buffers ( nframes_t buffer_size ) override;

    virtual bool bypass ( void ) const override
    {
        return *_bypass == 1.0f;
    }
    virtual void bypass ( bool v ) override;

private:

    void init ( void ) override;
    bool apply ( sample_t *buf, nframes_t nframes );
    void set_input_buffer ( int n, void *buf );
    void set_output_buffer ( int n, void *buf );
    void activate ( void );
    void deactivate ( void );
    bool loaded ( void ) const;

public:

    LADSPA_Plugin ( );
    virtual ~LADSPA_Plugin();

    nframes_t get_current_latency( void ) override;
    nframes_t get_module_latency ( void ) const override;
    void process ( nframes_t ) override;

    LOG_CREATE_FUNC( LADSPA_Plugin );
    MODULE_CLONE_FUNC( LADSPA_Plugin );

protected:

    bool plugin_instances ( unsigned int );
    void get ( Log_Entry &e ) const override;
    void set ( Log_Entry &e ) override;

};

#endif // LADSPA_SUPPORT

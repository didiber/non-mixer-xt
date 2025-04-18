/*******************************************************************************/
/* Copyright (C) 2005-2023, rncbc aka Rui Nuno Capela. All rights reserved.    */
/* Copyright (C) 2024- Stazed                                                  */
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
 * File:   VST3_Plugin.H
 * Author: sspresto
 *
 * Created on December 20, 2023, 9:24 AM
 */

#pragma once

#ifdef VST3_SUPPORT

#include <unordered_map>

#include "../Mixer_Strip.H"
#include "../Plugin_Module.H"
#include "Vst3_Impl.H"
#include "VST3PluginHost.H"
#include "runloop.h"

class EditorFrame;

extern std::string project_directory;
extern std::string export_import_strip;
extern std::vector<std::string>remove_custom_data_directories;

using namespace Steinberg;

class VST3_Plugin : public Plugin_Module
{
public:

    virtual bool load_plugin ( Module::Picked picked ) override;

    bool configure_inputs ( int ) override;
    void handle_port_connection_change ( void ) override;
    void handle_chain_name_changed ( void ) override;
    void handle_sample_rate_change ( nframes_t sample_rate ) override;
    void resize_buffers ( nframes_t buffer_size ) override;

    virtual bool bypass ( void ) const override
    {
        return *_bypass == 1.0f;
    }
    virtual void bypass ( bool v ) override;

    void freeze_ports ( void ) override;
    void thaw_ports ( void ) override;
    void clear_midi_vectors() override;

    void configure_midi_inputs () override;
    void configure_midi_outputs () override;

    nframes_t get_module_latency ( void ) const override;
    void process ( nframes_t ) override;

    LOG_CREATE_FUNC( VST3_Plugin );
    MODULE_CLONE_FUNC( VST3_Plugin );

    std::vector<Port> midi_input;
    std::vector<Port> midi_output;

    std::vector<std::string> _PresetList;

    // Set/add a parameter value/point.
    void setParameter (Vst::ParamID id, Vst::ParamValue value, uint32 offset);
    void set_control_value(unsigned long port_index, float value, bool update_custom_ui);

    // Parameter update methods - host to plugin from Module
    void updateParam(Vst::ParamID id, float fValue);

    // Parameters update methods - plugin to host
    void updateParamValues(bool update_custom_ui);
    // Get current parameter value.
    Vst::ParamValue getParameter (Vst::ParamID id) const;
    tresult notify (Vst::IMessage *message);

    // Open/close editor widget.
    bool try_custom_ui();
    bool init_custom_ui();
    // Editor controller methods.
    bool openEditor (void);
    void closeEditor (void);

    bool show_custom_ui();
    bool hide_custom_ui();
    EditorFrame * get_editor_frame()
    {
        return _pEditorFrame;
    }
    Vst::EditorHost::RunLoop* get_runloop()
    {
        return _pRunloop;
    }
    void set_visibility(bool vis)
    {
        _x_is_visible = vis;
    }

    void add_ntk_timer(int i_msecs);
    void remove_ntk_timer();
    void custom_update_ui_x();
    static void custom_update_ui ( void * );
    void update_controller_param ();

    int get_timer_msecs()
    {
        return _i_miliseconds;
    }

    Vst::ProcessContext *
    processContext( void )
    {
        return &_processContext;
    }

    // Save/Restore state
    bool load_state (VST3PluginHost::RAMStream& stream);
    bool save_state (VST3PluginHost::RAMStream& stream);
    void save_VST3_plugin_state(const std::string &filename);
    void restore_VST3_plugin_state(const std::string &filename);

    uint64_t getState (void** const dataPtr);
    void setProgram(int choice);

    VST3_Plugin();
    virtual ~VST3_Plugin();
private:

    // Project loading
    std::string _plugin_filename;
    std::string _sUniqueID;
    FUID _fuid;
    std::string _sName;
    void* _last_chunk;
    std::string _project_file;
    bool _found_plugin;

    // Instance variables.
    void *_pModule;

    class Handler;  // Forward declaration
    IPtr<Handler> _pHandler;

    IPtr<Vst::IComponent> _pComponent;
    IPtr<Vst::IEditController> _pController;
    IPtr<Vst::IUnitInfo> m_unitInfos;   // Not implemented

    Vst::IAudioProcessor *_pProcessor;
    Vst::ProcessContext _processContext;

    volatile bool _bProcessing;

    VST3IMPL::ParamChanges _cParams_in;
    VST3IMPL::ParamChanges _cParams_out;    // required by some DPF plugins

    std::map<Vst::ParamID, uint32_t> _ctrl_id_index;    // map of control out params, control index sync with below items
    std::vector<float>               _shadow_data;      // the control data value to update
    mutable std::vector<bool>        _update_ctrl;      // flag to indicate control needs updating

    Vst::ParameterInfo m_programParamInfo;

    VST3IMPL::EventList _cEvents_in;
    VST3IMPL::EventList _cEvents_out;

    // Processor buffers.
    Vst::AudioBusBuffers *_vst_buffers_in;
    Vst::AudioBusBuffers *_vst_buffers_out;

    // Processor data.
    Vst::ProcessData _vst_process_data;

    std::vector<int32_t> _vAudioInChannels;
    std::vector<int32_t> _vAudioOutChannels;
    int32_t _iAudioInBuses;
    int32_t _iAudioOutBuses;
    unsigned short _iMidiIns;
    unsigned short _iMidiOuts;

    float ** _audio_in_buffers;
    float ** _audio_out_buffers;

    bool _activated;
    bool _bEditor;  // unused??

    uint32_t  _position;        ///< Transport position in frames
    float     _bpm;             ///< Transport tempo in beats per minute
    bool      _rolling;         ///< Transport speed (0=stop, 1=play)

    // GUI Editor stuff...
    bool _bEditorCreated;
    bool _x_is_resizable;
    bool _x_is_visible;
    float _f_miliseconds;
    int _i_miliseconds;
    IPtr<IPlugView> _iPlugView;

    EditorFrame  *_pEditorFrame;
    Vst::EditorHost::RunLoop *_pRunloop;

    // Identififier-parameter map.
    std::unordered_map<uint32_t, unsigned long> _mParamIds;
    // Parameter finder (by id).
    unsigned long findParamId ( uint32_t id ) const;

    // File loader.
    bool find_vst_binary();
    bool open_file(const std::string& sFilename);
    bool open_descriptor(unsigned long iIndex);

    void close_descriptor();

    // Process functions
    void set_input_buffer ( int n, void *buf );
    void set_output_buffer ( int n, void *buf );
    bool loaded ( void ) const;
    bool process_reset();

    void process_jack_transport ( uint32_t nframes );
    void process_jack_midi_in ( uint32_t nframes, unsigned int port );
    void process_midi_in (unsigned char *data, unsigned int size,
                          unsigned long offset, unsigned short port);

    void process_jack_midi_out ( uint32_t nframes, unsigned int port );
    // Common host time-keeper process context.
    void updateProcessContext(jack_position_t &pos, const bool &xport_changed, const bool &has_bbt);
    // Cleanup.
    void clear_processContext( void );

    // Initialize create
    void initialize_plugin();
    void clear_plugin();
    int numChannels (Vst::MediaType type, Vst::BusDirection direction ) const;

    void create_audio_ports();
    void create_midi_ports();
    void create_control_ports();
    void activate ( void );
    void deactivate ( void );
    void vst3_activate ( Vst::IComponent *component,
                         Vst::MediaType type, Vst::BusDirection direction, bool state );
    void add_port ( const Port &p ) override;

    // MIDI controller assignment hash key/map.
    struct MidiMapKey
    {
        MidiMapKey (int16 po = 0, int16 ch = 0, int16 co = 0)
            : port(po), channel(ch), controller(co) {}
        MidiMapKey (const MidiMapKey& key)
            : port(key.port), channel(key.channel), controller(key.controller) {}

        bool operator< (const MidiMapKey& key) const
        {
            if (port != key.port)
                return (port < key.port);
            else if (channel != key.channel)
                return (channel < key.channel);
            else
                return (controller < key.controller);
        }

        int16 port;
        int16 channel;
        int16 controller;
    };

    std::map<MidiMapKey, Vst::ParamID> _mMidiMap;

protected:

    class Stream;

    // Save and restore
    void get ( Log_Entry &e ) const override;
    void set ( Log_Entry &e ) override;

};

#endif // VST3_SUPPORT

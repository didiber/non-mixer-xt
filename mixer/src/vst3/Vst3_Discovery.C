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
 * File:   Vst3_Discovery.C
 * Author: sspresto
 *
 * Created on December 16, 2023, 5:44 AM
 */

#ifdef VST3_SUPPORT

#include <dlfcn.h>      // dlopen, dlerror, dlsym
#include <cstring>      // strcmp
#include <algorithm>    // transform, toLower

#include "Vst3_Discovery.H"
#include "VST3_common.H"
#include "../../../nonlib/debug.h"

#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/gui/iplugview.h"

namespace vst3_discovery
{

static inline
std::string
getCategoryFromName( const std::string &name ) noexcept
{
    if ( name.empty ( ) )
        return "Unclassified";

    std::string sname ( name );

    std::transform ( sname.begin ( ), sname.end ( ), sname.begin ( ), [](unsigned char c)
    {
        return std::tolower ( c );
    } );

    // generic tags first
    if ( sname.find ( "delay" ) != std::string::npos )
        return "Time/Delays";
    if ( sname.find ( "reverb" ) != std::string::npos )
        return "Simulators/Reverbs";

    // filter
    if ( sname.find ( "filter" ) != std::string::npos )
        return "Frequency/Filters";

    // distortion
    if ( sname.find ( "distortion" ) != std::string::npos )
        return "Amplitude/Distortions";

    // dynamics
    if ( sname.find ( "dynamics" ) != std::string::npos )
        return "Amplitude/Dynamics";
    if ( sname.find ( "amplifier" ) != std::string::npos )
        return "Amplitude/Dynamics";
    if ( sname.find ( "compressor" ) != std::string::npos )
        return "Amplitude/Dynamics/Compressors";
    if ( sname.find ( "enhancer" ) != std::string::npos )
        return "Amplitude/Dynamics";
    if ( sname.find ( "exciter" ) != std::string::npos )
        return "Amplitude/Dynamics";
    if ( sname.find ( "gate" ) != std::string::npos )
        return "Amplitude/Dynamics";
    if ( sname.find ( "limiter" ) != std::string::npos )
        return "Amplitude/Dynamics/Limiters";

    // modulator
    if ( sname.find ( "modulator" ) != std::string::npos )
        return "Amplitude/Modulators";
    if ( sname.find ( "chorus" ) != std::string::npos )
        return "Amplitude/Modulators";
    if ( sname.find ( "flanger" ) != std::string::npos )
        return "Time/Flangers";
    if ( sname.find ( "phaser" ) != std::string::npos )
        return "Time/Phasers";
    if ( sname.find ( "saturator" ) != std::string::npos )
        return "Amplitude/Modulators";

    // utility
    if ( sname.find ( "utility" ) != std::string::npos )
        return "Utilities";
    if ( sname.find ( "analyzer" ) != std::string::npos )
        return "Analyser Plugin";
    if ( sname.find ( "converter" ) != std::string::npos )
        return "Utilities";
    if ( sname.find ( "deesser" ) != std::string::npos )
        return "Utilities";
    if ( sname.find ( "mixer" ) != std::string::npos )
        return "Utilities";

    // common tags
    if ( sname.find ( "verb" ) != std::string::npos )
        return "Simulators/Reverbs";

    if ( sname.find ( "eq" ) != std::string::npos )
        return "Frequency/EQs";

    if ( sname.find ( "tool" ) != std::string::npos )
        return "Utilities";

    // synth
    if ( sname.find ( "synth" ) != std::string::npos )
        return "Instrument Plugin";

    // other
    if ( sname.find ( "misc" ) != std::string::npos )
        return "Unclassified";
    if ( sname.find ( "other" ) != std::string::npos )
        return "Unclassified";

    return "Unclassified";
}

//-----------------------------------------------------------------------------

using namespace Steinberg;

//-----------------------------------------------------------------------------

class vst3_discovery_scan_host : public Vst::IHostApplication
{
public:

    vst3_discovery_scan_host( )
    {
        FUNKNOWN_CTOR
    }

    virtual
    ~vst3_discovery_scan_host( )
    {
        FUNKNOWN_DTOR
    }

    DECLARE_FUNKNOWN_METHODS

    //--- IHostApplication ----
    //
    tresult PLUGIN_API
    getName( Vst::String128 name ) override
    {
        const std::string host_name ( "NMXT-Plugin-Scan" );
        const std::u16string u16name = nmxt_common::utf8_to_utf16(host_name);
        const int c_size = u16name.size();
        const int nsize = ( c_size * 2 ) < 127 ? ( c_size * 2 ) : 127;

        ::memcpy ( name, u16name.c_str ( ), nsize - 1 );
        name[nsize] = 0;
        return kResultOk;
    }

    tresult PLUGIN_API
    createInstance( TUID /*cid*/, TUID /*_iid*/, void **obj ) override
    {
        *obj = nullptr;
        return kResultFalse;
    }

    FUnknown *
    get( )
    {
        return static_cast<Vst::IHostApplication *> ( this );
    }
};

tresult PLUGIN_API
vst3_discovery_scan_host::queryInterface(
    const char *_iid, void **obj )
{
    QUERY_INTERFACE ( _iid, obj, FUnknown::iid, IHostApplication )
    QUERY_INTERFACE ( _iid, obj, IHostApplication::iid, IHostApplication )

    * obj = nullptr;
    return kNoInterface;
}

uint32 PLUGIN_API
vst3_discovery_scan_host::addRef( void )
{
    return 1;
}

uint32 PLUGIN_API
vst3_discovery_scan_host::release( void )
{
    return 1;
}

static vst3_discovery_scan_host g_vst3HostContext;

//----------------------------------------------------------------------
// class vst3_discovery_scan::Impl -- VST3 plugin interface impl.
//

class vst3_discovery_scan::Impl
{
public:

    // Constructor.
    //    Impl() : m_module(nullptr), m_component(nullptr), m_controller(nullptr) {}

    Impl( ) : m_module( nullptr ), m_component( nullptr )
    {
    }

    // destructor.

    ~Impl( )
    {
        close_descriptor ( );
        close ( );
    }

    // File loader.

    bool
    open( const std::string& sFilename )
    {
        close ( );

        DMESSAGE ( "Open %s", sFilename.c_str ( ) );

        m_module = ::dlopen ( sFilename.c_str ( ), RTLD_LOCAL | RTLD_LAZY );
        if ( !m_module )
            return false;

        typedef bool (*VST3_ModuleEntry )(void *);
        const VST3_ModuleEntry module_entry
            = VST3_ModuleEntry ( ::dlsym ( m_module, "ModuleEntry" ) );

        if ( module_entry )
            module_entry ( m_module );

        return true;
    }

    bool
    open_descriptor( unsigned long iIndex )
    {
        if ( !m_module )
            return false;

        close_descriptor ( );

        typedef IPluginFactory * ( *VST3_GetFactory )( );
        const VST3_GetFactory get_plugin_factory
            = VST3_GetFactory ( ::dlsym ( m_module, "GetPluginFactory" ) );
        if ( !get_plugin_factory )
        {

            DMESSAGE ( "[%p]::open_descriptor(%lu)"
                " *** Failed to resolve plug-in factory.", this, iIndex );

            return false;
        }

        IPluginFactory *factory = get_plugin_factory ( );
        if ( !factory )
        {
            DMESSAGE ( "[%p]::open_descriptor(%lu)"
                " *** Failed to retrieve plug-in factory.", this, iIndex );

            return false;
        }

        PFactoryInfo FI;
        factory->getFactoryInfo ( &FI );

        m_factoryInfo = FI;

        IPluginFactory2 *factory2 = FUnknownPtr<IPluginFactory2> ( factory );
        IPluginFactory3 *factory3 = FUnknownPtr<IPluginFactory3> ( factory );

        if ( factory3 )
            factory3->setHostContext ( g_vst3HostContext.get ( ) );

        const int32 nclasses = factory->countClasses ( );

        unsigned long i = 0;

        for ( int32 n = 0; n < nclasses; ++n )
        {

            PClassInfo classInfo;
            if ( factory->getClassInfo ( n, &classInfo ) != kResultOk )
                continue;

            if ( ::strcmp ( classInfo.category, kVstAudioEffectClass ) )
                continue;

            if ( iIndex == i )
            {
                PClassInfoW classInfoW;
                if ( factory3 && factory3->getClassInfoUnicode ( n, &classInfoW ) == kResultOk )
                {
                    m_sSubCategories = classInfoW.subCategories;
                }
                else
                {
                    PClassInfo2 classInfo2;
                    if ( factory2 && factory2->getClassInfo2 ( n, &classInfo2 ) == kResultOk )
                    {
                        m_sSubCategories = classInfo2.subCategories;
                    }
                    else
                    {
                        m_sSubCategories = "Unclassified";
                    }
                }

                m_classInfo = classInfo;

                Vst::IComponent *component = nullptr;
                if ( factory->createInstance (
                    classInfo.cid, Vst::IComponent::iid,
                    (void **) &component ) != kResultOk )
                {
                    DMESSAGE ( "[%p]::open_descriptor(%lu)"
                        " *** Failed to create plug-in component.", this, iIndex );

                    return false;
                }

                m_component = owned ( component );

                if ( m_component->initialize ( g_vst3HostContext.get ( ) ) != kResultOk )
                {
                    DMESSAGE ( "[%p]::open_descriptor(%lu)"
                        " *** Failed to initialize plug-in component.", this, iIndex );
                    close_descriptor ( );
                    return false;
                }

#if 0   // We don't need to query editor or get controller for scan
                Vst::IEditController *controller = nullptr;
                if ( m_component->queryInterface (
                    Vst::IEditController::iid,
                    (void **) &controller ) != kResultOk )
                {
                    TUID controller_cid;
                    if ( m_component->getControllerClassId ( controller_cid ) == kResultOk )
                    {
                        if ( factory->createInstance (
                            controller_cid, Vst::IEditController::iid,
                            (void **) &controller ) != kResultOk )
                        {
                            DMESSAGE ( "Impl[%p]::open_descriptor(%lu)"
                                " *** Failed to create plug-in controller.", this, iIndex );
                        }
                        if ( controller &&
                            controller->initialize ( g_vst3HostContext.get ( ) ) != kResultOk )
                        {
                            DMESSAGE ( "[%p]::open_descriptor(%lu)"
                                " *** Failed to initialize plug-in controller.", this, iIndex );
                            controller = nullptr;
                        }
                    }
                }

                if ( controller ) m_controller = owned ( controller );

                // Connect components...
                if ( m_component && m_controller )
                {
                    FUnknownPtr<Vst::IConnectionPoint> component_cp ( m_component );
                    FUnknownPtr<Vst::IConnectionPoint> controller_cp ( m_controller );
                    if ( component_cp && controller_cp )
                    {
                        component_cp->connect ( controller_cp );
                        controller_cp->connect ( component_cp );
                    }
                }
#endif
                return true;
            }

            ++i;
        }

        return false;
    }

    void
    close_descriptor( )
    {
#if 0
        if ( m_component && m_controller )
        {
            FUnknownPtr<Vst::IConnectionPoint> component_cp ( m_component );
            FUnknownPtr<Vst::IConnectionPoint> controller_cp ( m_controller );
            if ( component_cp && controller_cp )
            {
                component_cp->disconnect ( controller_cp );
                controller_cp->disconnect ( component_cp );
            }
        }

        if ( m_component && m_controller &&
            FUnknownPtr<Vst::IEditController> ( m_component ).getInterface ( ) )
        {
            m_controller->terminate ( );
        }

        m_controller = nullptr;
#endif
        if ( m_component )
        {
            m_component->terminate ( );
            m_component = nullptr;
        }
    }

    void
    close( )
    {
        if ( !m_module )
            return;

        typedef void (*VST3_ModuleExit )( );
        const VST3_ModuleExit module_exit
            = VST3_ModuleExit ( ::dlsym ( m_module, "ModuleExit" ) );

        if ( module_exit )
            module_exit ( );

        ::dlclose ( m_module );
        m_module = nullptr;
    }

    // Accessors.

    Vst::IComponent *
    component( ) const
    {
        return m_component;
    }
#ifdef VST3_CONTROLLER

    Vst::IEditController *
    controller( ) const
    {
        return m_controller;
    }
#endif

    const PClassInfo&
    classInfo( ) const
    {
        return m_classInfo;
    }

    const PFactoryInfo&
    factoryInfo( ) const
    {
        return m_factoryInfo;
    }

    std::string
    subCategory( )
    {
        return m_sSubCategories;
    }

    int
    numChannels( Vst::MediaType type, Vst::BusDirection direction ) const
    {
        if ( !m_component )
            return -1;

        int nchannels = 0;

        const int32 nbuses = m_component->getBusCount ( type, direction );
        for ( int32 i = 0; i < nbuses; ++i )
        {
            Vst::BusInfo busInfo;
            if ( m_component->getBusInfo ( type, direction, i, busInfo ) == kResultOk )
            {
                if ( ( busInfo.busType == Vst::kMain ) ||
                    ( busInfo.flags & Vst::BusInfo::kDefaultActive ) )
                    nchannels += busInfo.channelCount;
            }
        }

        return nchannels;
    }

private:

    std::string m_sSubCategories;

    // Instance variables.
    void *m_module;

    PClassInfo m_classInfo;

    PFactoryInfo m_factoryInfo;

    IPtr<Vst::IComponent> m_component;
#ifdef VST3_CONTROLLER
    IPtr<Vst::IEditController> m_controller;
#endif
};

//----------------------------------------------------------------------
// class vst3_discovery_scan -- VST3 plugin interface
//

// Constructor.

vst3_discovery_scan::vst3_discovery_scan( void ) : m_pImpl( new Impl( ) )
{
    clear ( );
}

// destructor.

vst3_discovery_scan::~vst3_discovery_scan( void )
{
    close_descriptor ( );
    close ( );

    delete m_pImpl;
}

// File loader.

bool
vst3_discovery_scan::open( const std::string& sFilename )
{
    close ( );

    //    DMESSAGE("[%p]::open(\"%s\")", this, sFilename.c_str());

    return m_pImpl->open ( sFilename );
}

bool
vst3_discovery_scan::open_descriptor( unsigned long iIndex )
{
    close_descriptor ( );

    //  DMESSAGE("[%p]::open_descriptor( %lu)", this, iIndex);

    if ( !m_pImpl->open_descriptor ( iIndex ) )
        return false;

    const PClassInfo& classInfo = m_pImpl->classInfo ( );
    m_sName = classInfo.name;

    const PFactoryInfo& factoryInfo = m_pImpl->factoryInfo ( );
    m_sVendor = factoryInfo.vendor;

    m_sSubCategories = m_pImpl->subCategory ( );

    m_iUniqueID = nmxt_common::UIDtoString ( false, classInfo.cid );

    m_iAudioIns = m_pImpl->numChannels ( Vst::kAudio, Vst::kInput );
    m_iAudioOuts = m_pImpl->numChannels ( Vst::kAudio, Vst::kOutput );

#ifdef VST3_CONTROLLER

    m_iMidiIns = m_pImpl->component ( )->getBusCount ( Vst::kEvent, Vst::kInput );
    m_iMidiOuts = m_pImpl->component ( )->getBusCount ( Vst::kEvent, Vst::kOutput );

#if 0   // don't query parameters or editor
    Vst::IEditController *controller = m_pImpl->controller ( );
    if ( controller )
    {
        IPtr<IPlugView> editor
            = owned ( controller->createView ( Vst::ViewType::kEditor ) );
        m_bEditor = ( editor != nullptr );
    }

    m_iControlIns = 0;
    m_iControlOuts = 0;

    if ( controller )
    {
        const int32 nparams = controller->getParameterCount ( );
        for ( int32 i = 0; i < nparams; ++i )
        {
            Vst::ParameterInfo paramInfo;
            if ( controller->getParameterInfo ( i, paramInfo ) == kResultOk )
            {
                if ( paramInfo.flags & Vst::ParameterInfo::kIsReadOnly )
                    ++m_iControlOuts;
                else if ( paramInfo.flags & Vst::ParameterInfo::kCanAutomate )
                    ++m_iControlIns;
            }
        }
    }
#endif  // 0
#endif  // VST3_CONTROLLER
    return true;
}

// File unloader.

void
vst3_discovery_scan::close_descriptor( void )
{
    m_pImpl->close_descriptor ( );

    clear ( );
}

void
vst3_discovery_scan::close( void )
{
    //  DMESSAGE("vst3_discovery_scan[%p]::close()", this);

    m_pImpl->close ( );
}

#ifdef VST3_CONTROLLER
// Properties.

bool
vst3_discovery_scan::isOpen( void ) const
{
    return (m_pImpl->controller ( ) != nullptr );
}
#endif

// Cleaner/wiper.

void
vst3_discovery_scan::clear( void )
{
    m_sName.clear ( );
    m_sVendor.clear ( );
    m_sSubCategories.clear ( );

    m_iUniqueID = "";
    m_iAudioIns = 0;
    m_iAudioOuts = 0;
#ifdef VST3_CONTROLLER
    m_iControlIns = 0;
    m_iControlOuts = 0;
    m_iMidiIns = 0;
    m_iMidiOuts = 0;
    m_bEditor = false;
#endif
}

//-------------------------------------------------------------------------
// vst3_discovery_scan_file - The main scan procedure.
//

void
vst3_discovery_scan_file( const std::string& sFilename, std::list<Plugin_Info> & vst3pr )
{
    vst3_discovery_scan plugin;

    std::string sVst3Object = nmxt_common::get_vst3_object_file ( sFilename );

    if ( !plugin.open ( sVst3Object ) )
    {
        DMESSAGE ( "Could not open %s", sVst3Object.c_str ( ) );
        return;
    }

    unsigned long i = 0;
    while ( plugin.open_descriptor ( i ) )
    {
        Plugin_Info pi ( "VST3" );

        pi.name = plugin.name ( );
        pi.author = plugin.vendor ( );

        if ( std::strstr ( plugin.subCategory ( ).c_str ( ), "Instrument" ) != nullptr )
        {
            pi.category = "Instrument Plugin";
        }
        else
        {
            pi.category = getCategoryFromName ( pi.name );
        }

        pi.audio_inputs = plugin.audioIns ( );
        pi.audio_outputs = plugin.audioOuts ( );
        pi.midi_inputs = plugin.midiIns ( );
        pi.midi_outputs = plugin.midiOuts ( );

        pi.s_unique_id = plugin.uniqueID ( );
        pi.plug_path = sVst3Object;

        vst3pr.push_back ( pi );

        DMESSAGE ( "name = %s: category = %s: ID = %s: PATH = %s",
            pi.name.c_str ( ), pi.category.c_str ( ), pi.s_unique_id.c_str ( ), pi.plug_path.c_str ( ) );

        plugin.close_descriptor ( );
        ++i;
    }

    plugin.close ( );

    if ( i == 0 )
        WARNING ( "%s plugin file error" );
}

} // namespace vst3_discovery

#endif  // VST3_SUPPORT
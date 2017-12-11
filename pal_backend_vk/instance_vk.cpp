#include "pal_backend_vk/private/backend_private.h"

#include <iostream>
#include <iomanip>

// ----------------------------------------------------------------------

static PFN_vkCreateDebugReportCallbackEXT  pfn_vkCreateDebugReportCallbackEXT;
static PFN_vkDestroyDebugReportCallbackEXT pfn_vkDestroyDebugReportCallbackEXT;
static PFN_vkDebugReportMessageEXT         pfn_vkDebugReportMessageEXT;

void patchExtProcAddrs( pal_backend_vk_instance_o *obj ) {
	pfn_vkCreateDebugReportCallbackEXT  = ( PFN_vkCreateDebugReportCallbackEXT  ) obj->vkInstance.getProcAddr( "vkCreateDebugReportCallbackEXT" );
	pfn_vkDestroyDebugReportCallbackEXT = ( PFN_vkDestroyDebugReportCallbackEXT ) obj->vkInstance.getProcAddr( "vkDestroyDebugReportCallbackEXT" );
	pfn_vkDebugReportMessageEXT         = ( PFN_vkDebugReportMessageEXT         ) obj->vkInstance.getProcAddr( "vkDebugReportMessageEXT" );
	std::cout << "Patched proc addrs." << std::endl;
}

// ----------------------------------------------------------------------

static void create_debug_callback(pal_backend_vk_instance_o* obj);  // ffdecl.
static void destroy_debug_callback(pal_backend_vk_instance_o* obj);	// ffdecl.

// ----------------------------------------------------------------------

static VkBool32 debugCallback(
    VkDebugReportFlagsEXT      flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t                   object,
    size_t                     location,
    int32_t                    messageCode,
    const char *               pLayerPrefix,
    const char *               pMessage,
    void *                     pUserData ) {

	bool        shouldBailout = VK_FALSE;
	std::string logLevel      = "";

	if ( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
		logLevel = "INFO";
	} else if ( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
		logLevel = "WARN";
	} else if ( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) {
		logLevel = "PERF";
	} else if ( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
		logLevel = "ERROR";
		shouldBailout |= VK_TRUE;
	} else if ( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
		logLevel = "DEBUG";
	}

	std::ostringstream os;
	os << "\t " << std::left << std::setw( 8 ) << logLevel << "{" << std::setw( 10 ) << pLayerPrefix << "}: " << pMessage << std::endl;
	std::cout << os.str();

	// if error returns true, this layer will try to bail out and not forward the command
	return shouldBailout;
}

// ----------------------------------------------------------------------

static void create_debug_callback(pal_backend_vk_instance_o* obj){
	vk::DebugReportCallbackCreateInfoEXT debugCallbackCreateInfo;
	debugCallbackCreateInfo
	    .setPNext( nullptr )
	    .setFlags( ~vk::DebugReportFlagBitsEXT() )
	    .setPfnCallback( debugCallback )
	    .setPUserData( nullptr );

	obj->debugCallback = obj->vkInstance.createDebugReportCallbackEXT( debugCallbackCreateInfo );
}

// ----------------------------------------------------------------------

static void destroy_debug_callback(pal_backend_vk_instance_o * obj){
	obj->vkInstance.destroyDebugReportCallbackEXT( obj->debugCallback );
	obj->debugCallback = nullptr;
}

// ----------------------------------------------------------------------

pal_backend_vk_instance_o *create_instance( pal_backend_vk_api *api ) {

	auto obj = new pal_backend_vk_instance_o();

	vk::ApplicationInfo appInfo;
	appInfo
	    .setPApplicationName( "debug app" )
	    .setApplicationVersion( VK_MAKE_VERSION( 0, 0, 0 ) )
	    .setPEngineName( "project island" )
	    .setEngineVersion( VK_MAKE_VERSION( 0, 1, 0 ) )
	    .setApiVersion( VK_MAKE_VERSION( 1, 0, 46 ) );

	std::vector<const char *> instanceLayerNames     = {};
	std::vector<const char *> instanceExtensionNames = {};

	instanceExtensionNames.push_back( "VK_KHR_xcb_surface" );
	instanceExtensionNames.push_back( "VK_KHR_surface" );

	bool shouldDebug = true;

	if ( shouldDebug ) {
		instanceExtensionNames.push_back( VK_EXT_DEBUG_REPORT_EXTENSION_NAME );
		instanceLayerNames.push_back( "VK_LAYER_LUNARG_standard_validation" );
		instanceLayerNames.push_back( "VK_LAYER_LUNARG_object_tracker" );
	}

	vk::DebugReportCallbackCreateInfoEXT debugCallbackCreateInfo;
	debugCallbackCreateInfo
	    .setPNext( nullptr )
	    .setFlags( ~vk::DebugReportFlagBitsEXT() )
	    .setPfnCallback( debugCallback )
	    .setPUserData( nullptr );

	vk::InstanceCreateInfo info;
	info.setFlags( {} )
	    .setPNext( &debugCallbackCreateInfo )
	    .setPApplicationInfo( &appInfo )
	    .setEnabledLayerCount( uint32_t( instanceLayerNames.size() ) )
	    .setPpEnabledLayerNames( instanceLayerNames.data() )
	    .setEnabledExtensionCount( uint32_t( instanceExtensionNames.size() ) )
	    .setPpEnabledExtensionNames( instanceExtensionNames.data() );

	obj->vkInstance = vk::createInstance( info );

	api->cUniqueInstance = obj;

	patchExtProcAddrs( obj );

	create_debug_callback( obj );

	std::cout << "Instance created." << std::endl;
	return obj;
}

// ----------------------------------------------------------------------

void destroy_instance( pal_backend_vk_instance_o *obj ) {
	destroy_debug_callback(obj);
	obj->vkInstance.destroy();
	delete ( obj );
	std::cout << "Instance destroyed." << std::endl;
}

// ----------------------------------------------------------------------

void post_reload_hook( pal_backend_vk_instance_o *obj ) {
	std::cout << "** post reload hook triggered." << std::endl;
	patchExtProcAddrs( obj );
	destroy_debug_callback(obj);
	std::cout << "** Removed debug report callback." << std::endl;
	create_debug_callback(obj);
	std::cout << "** Added new debug report callback." << std::endl;
}

// ----------------------------------------------------------------------
// These method definitions are exported so that vkhpp can call them
// vkhpp has matching declarations
VkResult vkCreateDebugReportCallbackEXT(
    VkInstance                                instance,
    const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *             pAllocator,
    VkDebugReportCallbackEXT *                pCallback ) {
	return pfn_vkCreateDebugReportCallbackEXT(
	    instance,
	    pCreateInfo,
	    pAllocator,
	    pCallback );
}

void vkDestroyDebugReportCallbackEXT(
    VkInstance                   instance,
    VkDebugReportCallbackEXT     callback,
    const VkAllocationCallbacks *pAllocator ) {
	pfn_vkDestroyDebugReportCallbackEXT(
	    instance,
	    callback,
	    pAllocator );
}

void vkDebugReportMessageEXT(
    VkInstance                 instance,
    VkDebugReportFlagsEXT      flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t                   object,
    size_t                     location,
    int32_t                    messageCode,
    const char *               pLayerPrefix,
    const char *               pMessage ) {
	pfn_vkDebugReportMessageEXT(
	    instance,
	    flags,
	    objectType,
	    object,
	    location,
	    messageCode,
	    pLayerPrefix,
	    pMessage );
}

// ----------------------------------------------------------------------

# hides the symbols of a plugin
# except the Poco specific functions for bootstrapping the plugin
macro(pluginlib_hide_plugin_symbols package_name class_name)
  set(_version_script version__${package_name}__${class_name}.script)
  file(WRITE "${CMAKE_BINARY_DIR}/${_version_script}"
    "    {
      global:
        pocoBuildManifest;
        pocoInitializeLibrary;
        pocoUninitializeLibrary;
        pocoBuildManifest${package_name}__${class_name};
      local:
        *;
    };"
  )
  # checks if the linker supports version script
  include(TestCXXAcceptsFlag)
  check_cxx_accepts_flag("-Wl,--version-script,\"${CMAKE_BINARY_DIR}/${_version_script}\"" LD_ACCEPTS_VERSION_SCRIPT)
  if(LD_ACCEPTS_VERSION_SCRIPT)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-version-script=\"${CMAKE_BINARY_DIR}/${_version_script}\"")
  endif()
endmacro()

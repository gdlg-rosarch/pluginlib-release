/*********************************************************************
*
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Willow Garage, Inc. nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
* Author: Eitan Marder-Eppstein
*********************************************************************/

//NOTE: this should really never be included on its own, but just in case someone is bad we'll guard

#ifndef PLUGINLIB_CLASS_LOADER_IMP_H_
#define PLUGINLIB_CLASS_LOADER_IMP_H_

#include "boost/bind.hpp"
#include <list>
#include <stdexcept>

namespace pluginlib {
  template <class T>
  ClassLoader<T>::ClassLoader(std::string package, std::string base_class, std::string attrib_name) : package_(package), base_class_(base_class), attrib_name_(attrib_name)
  {
    classes_available_ = determineAvailableClasses();
  }

  template <class T>
  void ClassLoader<T>::loadLibraryForClass(const std::string & lookup_name)
  {
    std::string library_path;
    ClassMapIterator it = classes_available_.find(lookup_name);
    if (it != classes_available_.end()){
      library_path = it->second.library_path_;
    }
    else
    {
      throw LibraryLoadException(getErrorStringForUnknownClass(lookup_name));
    }
    library_path.append(Poco::SharedLibrary::suffix());
    try
    {
      ROS_DEBUG("Attempting to load library %s for class %s",
                library_path.c_str(), lookup_name.c_str());
      
      loadClassLibraryInternal(library_path, lookup_name);
    }
    catch (Poco::LibraryLoadException &ex)
    {
      std::string error_string = "Failed to load library " + library_path + ". Make sure that you are calling the PLUGINLIB_REGISTER_CLASS macro in the library code, and that names are consistent between this macro and your XML. Error string: " + ex.displayText();
      throw LibraryLoadException(error_string);
    }
    catch (Poco::NotFoundException &ex)
    {
      std::string error_string = "Failed to find library " + library_path + ". Are you sure that the library you need has been built? Error string: " + ex.displayText();
      throw LibraryLoadException(error_string);
    }
  }

  template <class T>
  int ClassLoader<T>::unloadLibraryForClass(const std::string& lookup_name)
  {
    ClassMapIterator it = classes_available_.find(lookup_name);
    if (it != classes_available_.end())
    {
      std::string library_path = it->second.library_path_;
      library_path.append(Poco::SharedLibrary::suffix());
      ROS_DEBUG("Attempting to unload library %s for class %s",
                library_path.c_str(), lookup_name.c_str());

      return unloadClassLibraryInternal(library_path);
    } else {
      throw LibraryUnloadException(getErrorStringForUnknownClass(lookup_name));
    }
  }

  template <class T>
  ClassLoader<T>::~ClassLoader()
  {
    for (LibraryCountMap::iterator it = loaded_libraries_.begin(); it != loaded_libraries_.end(); ++it)
    {
      if ( it->second > 0)
        unloadClassLibrary(it->first);
    }
  }


  template <class T>
  bool ClassLoader<T>::isClassLoaded(const std::string& lookup_name)
  {
    try
    {
      return poco_class_loader_.canCreate(getClassType(lookup_name));
    }
    catch (Poco::RuntimeException &ex)
    {
      return false;
    }
  }

  template <class T>
  std::vector<std::string> ClassLoader<T>::getDeclaredClasses()
  {
    std::vector<std::string> lookup_names;
    for (ClassMapIterator it = classes_available_.begin(); it != classes_available_.end(); ++it)
    {
      lookup_names.push_back(it->first);
    }
    return lookup_names;
  }

  template <class T>
  void ClassLoader<T>::refreshDeclaredClasses()
  {
    // determine classes not currently loaded for removal
    std::list<std::string> remove_classes;
    for (std::map<std::string, ClassDesc>::const_iterator it = classes_available_.begin(); it != classes_available_.end(); it++)
    {
      std::string library_path = it->second.library_path_;
      library_path.append(Poco::SharedLibrary::suffix());
      if (loaded_libraries_.find(library_path) == loaded_libraries_.end() || loaded_libraries_[library_path] == 0)
      {
        remove_classes.push_back(it->first);
      }
    }
    while (!remove_classes.empty())
    {
      classes_available_.erase(remove_classes.front());
      remove_classes.pop_front();
    }

    // add new classes
    std::map<std::string, ClassDesc> updated_classes = determineAvailableClasses();
    for (std::map<std::string, ClassDesc>::const_iterator it = updated_classes.begin(); it != updated_classes.end(); it++)
    {
      if (classes_available_.find(it->first) == classes_available_.end())
      {
        classes_available_.insert(std::pair<std::string, ClassDesc>(it->first, it->second));
      }
    }
  }

  template <class T>
  std::string ClassLoader<T>::getName(const std::string& lookup_name)
  {
    //remove the package name to get the raw plugin name
    std::vector<std::string> split;
    boost::split(split, lookup_name, boost::is_any_of("/"));
    return split.back();
  }

  template <class T>
  bool ClassLoader<T>::isClassAvailable(const std::string& lookup_name)
  {
    return classes_available_.find(lookup_name) != classes_available_.end();
  }

  template <class T>
  std::string ClassLoader<T>::getClassType(const std::string& lookup_name)
  {
    ClassMapIterator it = classes_available_.find(lookup_name);
    if (it != classes_available_.end())
      return it->second.derived_class_;
    return "";
  }

  template <class T>
  std::string ClassLoader<T>::getClassDescription(const std::string& lookup_name)
  {
    ClassMapIterator it = classes_available_.find(lookup_name);
    if (it != classes_available_.end())
      return it->second.description_;
    return "";
  }

  template <class T>
  std::string ClassLoader<T>::getBaseClassType() const
  {
    return base_class_;
  }

  template <class T>
  std::string ClassLoader<T>::getClassLibraryPath(const std::string& lookup_name)
  {
    ClassMapIterator it = classes_available_.find(lookup_name);
    if (it != classes_available_.end())
      return it->second.library_path_;
    return "";
  }

  template <class T>
  std::string ClassLoader<T>::getClassPackage(const std::string& lookup_name)
  {
    ClassMapIterator it = classes_available_.find(lookup_name);
    if (it != classes_available_.end())
      return it->second.package_;
    return "";
  }

  template <class T>
  std::string ClassLoader<T>::getPluginManifestPath(const std::string& lookup_name)
  {
    ClassMapIterator it = classes_available_.find(lookup_name);
    if (it != classes_available_.end())
      return it->second.plugin_manifest_path_;
    return "";
  }

  template <class T>
  T* ClassLoader<T>::createClassInstance(const std::string& lookup_name, bool auto_load)
  {
    if(auto_load && !isClassLoaded(lookup_name))
      loadLibraryForClass(lookup_name);

    try{
      return poco_class_loader_.create(getClassType(lookup_name));
    }
    catch(const Poco::RuntimeException& ex){
      std::string error_string = "The class " + lookup_name + " could not be loaded. Error: " + ex.displayText();
      throw CreateClassException(error_string);
    }
  }
  
  template <class T>
  boost::shared_ptr<T> ClassLoader<T>::createInstance(const std::string& lookup_name)
  {
    T* instance = createUnmanagedInstance(lookup_name);
    return boost::shared_ptr<T>(instance, boost::bind(&ClassLoader<T>::garbageInstance, this, _1, lookup_name));
  }

  template <class T>
  T* ClassLoader<T>::createUnmanagedInstance(const std::string& lookup_name)
  {
    loadLibraryForClass(lookup_name);

    T* instance = 0;
    try{
      instance = poco_class_loader_.create(getClassType(lookup_name));
    }
    catch(const Poco::RuntimeException& ex){
      std::string error_string = "The class " + lookup_name + " could not be loaded. Error: " + ex.displayText();
      // call unload library to keep load/unload counting consistent
      unloadLibraryForClass(lookup_name);
      throw CreateClassException(error_string);
    }
    return instance;
  }

  template <class T>
  void ClassLoader<T>::garbageInstance(T* p, const std::string& lookup_name)
  {
    if (p)
    {
      delete p;
      p = 0;
    }
    unloadLibraryForClass(lookup_name);
  }

  template <class T>
  bool ClassLoader<T>::unloadClassLibrary(const std::string& library_path)
  {
    LibraryCountMap::iterator it = loaded_libraries_.find(library_path);
    if (it == loaded_libraries_.end())
    {
      ROS_DEBUG("unable to unload library which is not loaded");
      return false;
    }
    else if (it-> second > 1)
      (it->second)--;
    else
      poco_class_loader_.unloadLibrary(library_path);

    return true;

  }

  template <class T>
  bool ClassLoader<T>::loadClassLibrary(const std::string& library_path){
    try
    {
      loadClassLibraryInternal(library_path);
    }
    catch (Poco::LibraryLoadException &ex)
    {
      return false;
    }
    catch (Poco::NotFoundException &ex)
    {
      return false;
    }
    return true;
  }

  template <class T>
  void ClassLoader<T>::loadClassLibraryInternal(const std::string& library_path, const std::string& list_name_arg) {
    std::string list_name = list_name_arg;
    boost::replace_first(list_name, "/", "__"); 

    poco_class_loader_.loadLibrary(library_path, list_name);
    LibraryCountMap::iterator it = loaded_libraries_.find(library_path);
    if (it == loaded_libraries_.end())
      loaded_libraries_[library_path] = 1;  //for correct destruction and access
    else
      loaded_libraries_[library_path] = loaded_libraries_[library_path] + 1;
  }

  template <class T>
  int ClassLoader<T>::unloadClassLibraryInternal(const std::string& library_path) {
    LibraryCountMap::iterator it = loaded_libraries_.find(library_path);
    if (it != loaded_libraries_.end() && loaded_libraries_[library_path] > 0)
    {
      loaded_libraries_[library_path]--;
      if (loaded_libraries_[library_path] == 0)
        poco_class_loader_.unloadLibrary(library_path);
      return loaded_libraries_[library_path];
    }
    else
    {
      std::string error_string = "Failed to unload library " + library_path + ". The library was not loaded before or might have already been unloaded.";
      throw LibraryUnloadException(error_string);
    }
  }

  template <class T>
  std::vector<std::string> ClassLoader<T>::getClassesInLibrary(const std::string & library_path)
  {
    std::vector<std::string> lookup_names;


    const Poco::Manifest<T> * manifest = poco_class_loader_.findManifest(library_path);
    if (manifest == NULL)
      return lookup_names;

    for (typename Poco::Manifest<T>::Iterator it = manifest->begin(); it != manifest->end(); ++it)
    {
      std::string name = it->name();
      boost::replace_first(name, "__", "/"); 
      lookup_names.push_back(name);
    }
    return lookup_names;
  }

  template <class T>
  std::vector<std::string> ClassLoader<T>::getRegisteredLibraries()
  {
    std::vector<std::string> library_names;
    for (ClassMapIterator it = classes_available_.begin(); it != classes_available_.end(); it++){
      bool duplicate = false;
      for (unsigned int i=0; i<library_names.size(); i++)
        if (it->second.library_path_ == library_names[i])
          duplicate = true;
      if (!duplicate)
        library_names.push_back(it->second.library_path_);
    }
    return library_names;
  }


  template <class T>
  std::vector<std::string> ClassLoader<T>::getLoadedLibraries()
  {
    std::vector<std::string> library_names;

    /*
       \todo find a way to get ths out of poco
       for (typename Poco::ClassLoader<T>::Iterator it = poco_class_loader_.begin(); it != poco_class_loader_.end(); ++it)
       {
       library_names.push_back(it->second->className());
       }
       return library_names;
       */
    LibraryCountMap::iterator it;
    for (it = loaded_libraries_.begin(); it != loaded_libraries_.end(); it++)
    {
      if (it->second > 0)
        library_names.push_back(it->first);
    }
    return library_names;
  }

  template <class T>
  std::map<std::string, ClassDesc> ClassLoader<T>::determineAvailableClasses()
  {
    std::map<std::string, ClassDesc> classes_available;
    //Pull possible files from manifests of packages which depend on this package and export class
    std::vector<std::string> paths;
    ros::package::getPlugins(package_, attrib_name_, paths);
    if (paths.size() == 0)
    {
      std::string error_string = "rospack could not find the " + package_ + " package containing " +  base_class_;
      throw LibraryLoadException(error_string);
    }

    //The poco factory for base class T
    for (std::vector<std::string>::iterator it = paths.begin(); it != paths.end(); ++it)
    {
      TiXmlDocument document;
      document.LoadFile(*it);
      TiXmlElement * config = document.RootElement();
      if (config == NULL)
      {
        ROS_ERROR("Skipping XML Document \"%s\" which had no Root Element.  This likely means the XML is malformed or missing.", it->c_str());
        continue;
      }
      if (config->ValueStr() != "library" &&
          config->ValueStr() != "class_libraries")
      {
        ROS_ERROR("The XML document \"%s\" given to add must have either \"library\" or \
            \"class_libraries\" as the root tag", it->c_str());
        continue;
      }
      //Step into the filter list if necessary
      if (config->ValueStr() == "class_libraries")
      {
        config = config->FirstChildElement("library");
      }

      TiXmlElement* library = config;
      while ( library != NULL)
      {
        std::string library_path = library->Attribute("path");
        if (library_path.size() == 0)
        {
          ROS_ERROR("Failed to find Path Attirbute in library element in %s", it->c_str());
          continue;
        }

        std::string package_name = pluginlib::getPackageFromLibraryPath(*it);
        if (package_name == "")
          ROS_ERROR("Could not find package name for class %s", it->c_str());

        std::string parent_dir = ros::package::getPath(package_name);
        std::string full_library_path = joinPaths(parent_dir , library_path);

        TiXmlElement* class_element = library->FirstChildElement("class");
        while (class_element)
        {
          std::string base_class_type = class_element->Attribute("base_class_type");
          std::string lookup_name = class_element->Attribute("name");
          std::string derived_class = class_element->Attribute("type");

          //make sure that this class is of the right type before registering it
          if(base_class_type == base_class_){

            // register class here
            TiXmlElement* description = class_element->FirstChildElement("description");
            std::string description_str;
            if (description)
              description_str = description->GetText() ? description->GetText() : "";
            else
              description_str = "No 'description' tag for this plugin in plugin description file.";

            classes_available.insert(std::pair<std::string, ClassDesc>(lookup_name, ClassDesc(lookup_name, derived_class, base_class_type, package_name, description_str, full_library_path, *it)));
            ROS_DEBUG("MATCHED Base type for class with name: %s type: %s base_class_type: %s Expecting base_class_type %s",
                      lookup_name.c_str(), derived_class.c_str(), base_class_type.c_str(), base_class_.c_str());
          }
          else
          {
            ROS_DEBUG("UNMATCHED Base type for class with name: %s type: %s base_class_type: %s Expecting base_class_type %s",
                      lookup_name.c_str(), derived_class.c_str(), base_class_type.c_str(), base_class_.c_str());

          }
          //step to next class_element
          class_element = class_element->NextSiblingElement( "class" );
        }
        library = library->NextSiblingElement( "library" );
      }
    }
    return classes_available;
  }

  template <class T>
  std::string ClassLoader<T>::getErrorStringForUnknownClass(const std::string& lookup_name)
  {
    std::string declared_types;
    std::vector<std::string> types = getDeclaredClasses();
    for ( unsigned int i = 0; i < types.size(); i ++)
    {
      declared_types = declared_types + std::string(" ") + types[i];
    }
    return "According to the loaded plugin descriptions the class " + lookup_name 
      + " with base class type " + base_class_ + " does not exist. Declared types are " + declared_types;
  }
};

#endif

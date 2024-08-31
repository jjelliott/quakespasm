#This file is used for "faking" results from what would be PkgConfig on Linux
#and creating these library targets from the bundled codecs.
#I think there is some way to have PkgConfig on Windows also so we check each
#library before defining it and see if it's already found
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(CODECS_LIB_PATH x64)
else()
	set(CODECS_LIB_PATH x86)
endif()

set(CODECS_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Windows/codecs)
set(CODECS_LIBRARY_DIR ${CODECS_BASE_DIR}/${CODECS_LIB_PATH})

find_file(OGG_DLL NAMES libogg-0.dll PATHS ${CODECS_LIBRARY_DIR})
add_library(PC_OGG SHARED IMPORTED)
set_target_properties(PC_OGG PROPERTIES IMPORTED_LOCATION "${OGG_DLL}")

if (NOT PC_MPG123_FOUND)
	find_path(MPG123_INCLUDE_DIR mpg123.h PATH_SUFFIXES include PATHS ${CODECS_BASE_DIR})
	find_library(MPG123_LIBRARY NAMES libmpg123 libmpg123.dll	PATH_SUFFIXES ${CODECS_LIB_PATH} PATHS ${CODECS_BASE_DIR})
	find_file(MPG123_DLL NAMES libmpg123-0.dll PATHS ${CODECS_LIBRARY_DIR})

	if (MPG123_LIBRARY)
		set(PC_MPG123_FOUND TRUE)
		set(PC_MPG123_INCLUDE_DIRS ${MPG123_INCLUDE_DIR})
		set(PC_MPG123_LIBRARY_DIRS ${CODECS_LIBRARY_DIR})
		set(PC_MPG123_LIBRARIES ${MPG123_LIBRARY})

		add_library(PkgConfig::PC_MPG123 SHARED IMPORTED GLOBAL)
		set_target_properties(PkgConfig::PC_MPG123 PROPERTIES IMPORTED_IMPLIB "${MPG123_LIBRARY}" IMPORTED_LOCATION "${MPG123_DLL}" INTERFACE_INCLUDE_DIRECTORIES "${MPG123_INCLUDE_DIR}")
	endif()
endif()

if (NOT PC_MAD_FOUND)
	find_path(MAD_INCLUDE_DIR mad.h PATH_SUFFIXES include PATHS ${CODECS_BASE_DIR})
	find_library(MAD_LIBRARY NAMES libmad libmad.dll PATH_SUFFIXES ${CODECS_LIB_PATH} PATHS ${CODECS_BASE_DIR})
	find_file(MAD_DLL NAMES libmad-0.dll PATHS ${CODECS_LIBRARY_DIR})

	if (MAD_LIBRARY)
		set(PC_MAD_FOUND TRUE)
		set(PC_MAD_INCLUDE_DIRS ${MAD_INCLUDE_DIR})
		set(PC_MAD_LIBRARY_DIRS ${CODECS_LIBRARY_DIR})
		set(PC_MAD_LIBRARIES ${MAD_LIBRARY})

		add_library(PkgConfig::PC_MAD SHARED IMPORTED GLOBAL)
		set_target_properties(PkgConfig::PC_MAD PROPERTIES IMPORTED_IMPLIB "${MAD_LIBRARY}" IMPORTED_LOCATION "${MAD_DLL}" INTERFACE_INCLUDE_DIRECTORIES "${MAD_INCLUDE_DIR}")
	endif()
endif()

if (NOT PC_OPUSFILE_FOUND)
	find_path(OPUSFILE_INCLUDE_DIR opusfile.h PATH_SUFFIXES include/opus PATHS ${CODECS_BASE_DIR})
	find_library(OPUSFILE_LIBRARY NAMES libopusfile libopusfile.dll PATH_SUFFIXES ${CODECS_LIB_PATH} PATHS ${CODECS_BASE_DIR})
	find_file(OPUSFILE_DLL NAMES libopusfile-0.dll PATHS ${CODECS_LIBRARY_DIR})
	find_file(OPUS_DLL NAMES libopus-0.dll PATHS ${CODECS_LIBRARY_DIR})

	if (OPUSFILE_LIBRARY)
		set(PC_OPUSFILE_FOUND TRUE)
		set(PC_OPUSFILE_INCLUDE_DIRS ${OPUSFILE_INCLUDE_DIR})
		set(PC_OPUSFILE_LIBRARY_DIRS ${CODECS_LIBRARY_DIR})
		set(PC_OPUSFILE_LIBRARIES ${OPUSFILE_LIBRARY})

		add_library(PkgConfig::PC_OPUSFILE SHARED IMPORTED GLOBAL)
		set_target_properties(PkgConfig::PC_OPUSFILE PROPERTIES IMPORTED_IMPLIB "${OPUSFILE_LIBRARY}" IMPORTED_LOCATION "${OPUSFILE_DLL}" INTERFACE_INCLUDE_DIRECTORIES "${OPUSFILE_INCLUDE_DIR}")
		add_library(PC_OPUS SHARED IMPORTED)
		set_target_properties(PC_OPUS PROPERTIES IMPORTED_LOCATION "${OPUS_DLL}")
	endif()
endif()

if (NOT PC_FLAC_FOUND)
	find_path(FLAC_INCLUDE_DIR stream_decoder.h PATH_SUFFIXES include/FLAC PATHS ${CODECS_BASE_DIR})
	find_library(FLAC_LIBRARY NAMES libFLAC libFLAC.dll PATH_SUFFIXES ${CODECS_LIB_PATH} PATHS ${CODECS_BASE_DIR})
	find_file(FLAC_DLL NAMES libflac-8.dll PATHS ${CODECS_LIBRARY_DIR})

	if (FLAC_LIBRARY)
		set(PC_FLAC_FOUND TRUE)
		set(PC_FLAC_INCLUDE_DIRS ${FLAC_INCLUDE_DIR})
		set(PC_FLAC_LIBRARY_DIRS ${FLAC_LIBRARY_DIR})
		set(PC_FLAC_LIBRARIES ${FLAC_LIBRARY})

		add_library(PkgConfig::PC_FLAC SHARED IMPORTED GLOBAL)
		set_target_properties(PkgConfig::PC_FLAC PROPERTIES IMPORTED_IMPLIB "${FLAC_LIBRARY}" IMPORTED_LOCATION "${FLAC_DLL}" INTERFACE_INCLUDE_DIRECTORIES "${FLAC_INCLUDE_DIR}")
	endif()
endif()

if (NOT PC_VORBISFILE_FOUND)
	find_path(VORBISFILE_INCLUDE_DIR vorbisfile.h PATH_SUFFIXES include/vorbis PATHS ${CODECS_BASE_DIR})
	find_library(VORBISFILE_LIBRARY NAMES libvorbisfile libvorbisfile.dll PATH_SUFFIXES ${CODECS_LIB_PATH} PATHS ${CODECS_BASE_DIR})
	find_file(VORBISFILE_DLL NAMES libvorbisfile-3.dll PATHS ${CODECS_LIBRARY_DIR})
	find_file(VORBIS_DLL NAMES libvorbis-0.dll PATHS ${CODECS_LIBRARY_DIR})

	if (VORBISFILE_LIBRARY)
		set(PC_VORBISFILE_FOUND TRUE)
		set(PC_VORBISFILE_INCLUDE_DIRS ${VORBISFILE_INCLUDE_DIR})
		set(PC_VORBISFILE_LIBRARY_DIRS ${VORBISFILE_LIBRARY_DIR})
		set(PC_VORBISFILE_LIBRARIES ${VORBISFILE_LIBRARY})

		add_library(PkgConfig::PC_VORBISFILE SHARED IMPORTED GLOBAL)
		set_target_properties(PkgConfig::PC_VORBISFILE PROPERTIES IMPORTED_IMPLIB "${VORBISFILE_LIBRARY}" IMPORTED_LOCATION "${VORBISFILE_DLL}" INTERFACE_INCLUDE_DIRECTORIES "${VORBISFILE_INCLUDE_DIR}")
		add_library(PC_VORBIS SHARED IMPORTED)
		set_target_properties(PC_VORBIS PROPERTIES IMPORTED_LOCATION "${VORBIS_DLL}")
	endif()
endif()

if (NOT PC_MIKMOD_FOUND)
	find_path(MIKMOD_INCLUDE_DIR mikmod.h PATH_SUFFIXES include PATHS ${CODECS_BASE_DIR})
	find_library(MIKMOD_LIBRARY NAMES libmikmod libmikmod.dll PATH_SUFFIXES ${CODECS_LIB_PATH} PATHS ${CODECS_BASE_DIR})
	find_file(MIKMOD_DLL NAMES libmikmod-3.dll PATHS ${CODECS_LIBRARY_DIR})

	if (MIKMOD_LIBRARY)
		set(PC_MIKMOD_FOUND TRUE)
		set(PC_MIKMOD_INCLUDE_DIRS ${MIKMOD_INCLUDE_DIR})
		set(PC_MIKMOD_LIBRARY_DIRS ${MIKMOD_LIBRARY_DIR})
		set(PC_MIKMOD_LIBRARIES ${MIKMOD_LIBRARY})

		add_library(PkgConfig::PC_MIKMOD SHARED IMPORTED GLOBAL)
		set_target_properties(PkgConfig::PC_MIKMOD PROPERTIES IMPORTED_IMPLIB "${MIKMOD_LIBRARY}" IMPORTED_LOCATION "${MIKMOD_DLL}" INTERFACE_INCLUDE_DIRECTORIES "${MIKMOD_INCLUDE_DIR}")
	endif()
endif()

if (NOT PC_XMP_FOUND)
	find_path(XMP_INCLUDE_DIR xmp.h PATH_SUFFIXES include PATHS ${CODECS_BASE_DIR})
	find_library(XMP_LIBRARY NAMES libxmp libxmp.dll PATH_SUFFIXES ${CODECS_LIB_PATH} PATHS ${CODECS_BASE_DIR})
	find_file(XMP_DLL NAMES libxmp.dll PATHS ${CODECS_LIBRARY_DIR})

	if (XMP_LIBRARY)
		set(PC_XMP_FOUND TRUE)
		set(PC_XMP_INCLUDE_DIRS ${XMP_INCLUDE_DIR})
		set(PC_XMP_LIBRARY_DIRS ${XMP_LIBRARY_DIR})
		set(PC_XMP_LIBRARIES ${XMP_LIBRARY})

		add_library(PkgConfig::PC_XMP SHARED IMPORTED GLOBAL)
		set_target_properties(PkgConfig::PC_XMP PROPERTIES IMPORTED_IMPLIB "${XMP_LIBRARY}" IMPORTED_LOCATION "${XMP_DLL}" INTERFACE_INCLUDE_DIRECTORIES "${XMP_INCLUDE_DIR}")
	endif()
endif()
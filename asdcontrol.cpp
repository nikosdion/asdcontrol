/*
 * ASDControl -- Apple Studio Display Brightness Control
 * Copyright (c) 2023 Nicholas K. Dionysopoulos
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <sys/signal.h>
#include <getopt.h>
#include <linux/hiddev.h>

#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <list>

using namespace std;

// Limits for the HID device feature detection loops
#define HID_MAX_USAGES			1024
#define HID_MAX_APPLICATIONS	16

// Current version
#define VERSION "0.4"

// Operation modes for this software
const int USAGE_MODE_GET = 0;
const int USAGE_MODE_SET = 1;
const int USAGE_MODE_DETECT = 2;
const int USAGE_MODE_SETREL = 3;

// USB HID report ID for the monitor's brightness
const int BRIGHTNESS_CONTROL              = 1;
// USB HID usage code for setting the brightness
const int USAGE_CODE                      = 0x820001;

// Supported vendors
const int APPLE                           = 0x05ac;

// Supported monitors
const int STUDIO_DISPLAY_27               = 0x1114;

// Forward Declarations
void init_device_database();
void dump_supported();

// Helpful declarations
typedef unsigned Vendor;
typedef unsigned Product;

struct DeviceId {
  Product product;
  Vendor vendor;
  string description;
  int brightness_min;
  int brightness_max;

  DeviceId ( Vendor vendor_, Product product_, string description_,
		 int brightness_min = 0, int brightness_max = 255 )
    : product( product_ )
    , vendor( vendor_ )
    , description( description_ )
	, brightness_min(brightness_min)
	, brightness_max(brightness_max)
	{ }

  bool operator < ( const DeviceId& other ) const {
    return (vendor < other.vendor) || 
      (vendor == other.vendor && product < other.product );
  }
};

typedef set< DeviceId > SupportedDevices;
SupportedDevices supportedDevices;

typedef map< Vendor, string > SupportedVendors;
SupportedVendors supportedVendors;

typedef pair< Vendor, string> VendorDesc;


/**
 * Does it look like a number?
 *
 * @return Whether the string looks like an integer, or a relative number.
 */
bool number( const char* str ){
  if (!str ) return false;

  if (!(((*str >= '0') && (*str <= '9')) || (*str == '+') || (*str == '-')))
  {
    return false;
  }

  for (char c = *str; c; c=*++str)
  {
    if (!((c >= '0') && (c <= '9')))
    {
      return false;
    }

  }
  
  return true;
}

/** @return a non-NULL DeviceID ptr if the device is in our database */
const DeviceId* is_supported ( const hiddev_devinfo& device_info ) {
  Product product = device_info.product & 0xFFFF;
  Vendor vendor = device_info.vendor & 0xFFFF;

  SupportedDevices::const_iterator i = supportedDevices.find( DeviceId( vendor, product, "" ));
  if (i != supportedDevices.end())
	  return &*i;
  return 0;
}

/**
 * @param v query vendor
 * @param p query product
 * @return description of the device with given vendor and product 
 */
string description ( Vendor v, Product p ) {
  SupportedDevices::iterator it = 
    supportedDevices.find( DeviceId( v, p, "" ));

  if ( it != supportedDevices.end() )
    return it->description;

  return "";
}

/** @param v vendor to query
 * @return true if the venodr is in the database 
 */
bool known_vendor ( Vendor v ) {
  v &= 0xFFFF;
  return supportedVendors.find( v ) != supportedVendors.end();
}

/** Checks whether the HID device is a usb monitor 
 * @param device_info HID device info
 * @param fd file to read applications from
 */
bool is_usb_monitor ( const hiddev_devinfo& device_info, int fd ) {
  
  /* Now that we have the number of applications, we can retrieve them */
  /* using the HIDIOCAPPLICATION ioctl() call */
  /* applications are indexed from 0..{num_applications-1} */
  for ( int appl_num = 0; appl_num < device_info.num_applications; 
        ++appl_num ) {
    int application = ioctl( fd, HIDIOCAPPLICATION, appl_num );
    /* See https://usb.org/document-library/hid-usage-tables-14 for the source of 0x80 */
    if ( ((application >> 16) & 0xFF) == 0x80 ) {
      return true;
    }
  }
  return false;
}

/** Pretty-prints the given device information
 * @param o output stream to print to
 * @param device_info HID device info
 */
void format_device( ostream& o, const hiddev_devinfo& device_info ) {
  Vendor  v = device_info.vendor & 0xFFFF;
  Product p = device_info.product & 0xFFFF;
  o << "Vendor=" << showbase << setw( 6 ) << hex << v;
  if ( known_vendor( v ) )
    o << " (" << supportedVendors[ v ] << ")";
  
  o << ", Product=" << showbase << setw( 6 ) << hex << p ;

  if ( is_supported( device_info )) 
    o << "[" << description( v, p ) << "]";

  o << endl;
}


/** Debug routine for dumping device usage 
 * @param usage_ref usage description 
 */
void dump_usage ( hiddev_usage_ref& usage_ref ) {
  printf ("  report_type   =%d\n", usage_ref.report_type );
  printf ("  report_id     =%d\n", usage_ref.report_id );
  printf ("  field_index   =%d\n", usage_ref.field_index );
  printf ("  usage_index   =%d\n", usage_ref.usage_index );
  printf ("  usage_code    =%d\n", usage_ref.usage_code );
}

/** Prints help for the program.
 * @param programName this program name
 */
void help( const char *programName ) {
  printf( "asdcontrol " VERSION "\n");

  printf( "USAGE: %s [--silent|-s] [--brief|-b] [--help|-h] [--about|-a] "
          "[--detect|-d] [--list-all |-l] <hid device(s)> [<brightness>]\n\n"
          "Parameters:\n"
          "  --silent,-s\n"
          "         Suppress non-functional program output\n"
          "  --brief,-b\n"
          "         Print brightness value only when in query mode,\n"
          "         otherwise ignored.\n"
          "  --C, -d\n"
          "         Perform detection only\n"
          "  --list-all, -l\n"
          "         List supported devices and exit\n"
          "  --help,-h\n"
          "         Show short help message and quit.\n"
          "  --about,-a\n"
          "         Show information about the program, some credits and thanks.\n"
          "  hid device\n"
          "         device that represents your Apple Studio Display.\n"
          "         It should normally be one of /dev/usb/hiddevX\n"
          "      Note\n"
          "         You must have write permissions to this device.\n"
          "      Note\n"
          "         It should be safe to run the program on other device than\n"
          "         Apple Studio Display as the program checks whether\n"
          "         the device is Apple display and warns about it.\n"
          "  brightness\n"
          "         When this option is specified, the operation is to set brightness,\n"
          "         otherwise, current brightness is retrieved. If brightness starts\n"
          "         with '+' or '-', the current brightness is increased or decreased\n"
          "         by that value.\n"
          "      Note\n"
          "         Precede brightness decrement by '--'.\n"
          "      See also: --brief option.\n"

          "\n\nEXAMPLES:\n\n"
          "In the following examples I assume that your HID device is /dev/usb/hiddev0. You may \n"
          "have it as /dev/hiddevX or /dev/usb/hiddevX.\n"
          "\n"
          "  asdcontrol\n"
          "  asdcontrol --help\n"
          "      Show this help message.\n"
          "\n"
          "  asdcontrol --detect /dev/usb/hiddev*\n"
          "      Perform detection, which HID device is actually your display to be controlled.\n"
          "\n"
          "  asdcontrol /dev/usb/hiddev0\n"
          "      Read current brightness parameter\n"
          "\n"
          "  asdcontrol /dev/usb/hiddev0 20000\n"
          "      Set brightness to 160. Note, that brightness setting depends on your model. \n"
          "      Generally, this parameter may get values in the range [0-65536].\n"
          "\n"
          "  asdcontrol /dev/usb/hiddev0 +100\n"
          "      Increment current brightness by 100.\n"
          "\n"
          "  asdcontrol /dev/usb/hiddev0 -- -100\n"
          "      Decrement current brightness by 100. Please,note '--'!\n"
          ,
      
          programName );
}

/** Prints brief notice about the program */
void notice() {
  printf( "ASDControl " VERSION " -- Apple Studio Display Brightness Control\n" );
  printf( "Copyright (c) 2023 Nicholas K. Dionysopoulos\n\n" );
}

/** Prints detailed info about this program */
void about() {
  printf( "ASDControl " VERSION " -- Apple Studio Display Brightness Control\n"
          "Copyright (c) 2023 Nicholas K. Dionysopoulos\n\n"

          "This program is free software; you can redistribute it and/or modify\n"
          "it under the terms of the GNU General Public License as published by\n"
          "the Free Software Foundation; either version 2 of the License, or\n"
          "(at your option) any later version.\n\n"

          "This program is distributed in the hope that it will be useful,\n"
          "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
          "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
          "GNU General Public License for more details.\n\n"

          "You should have received a copy of the GNU General Public License\n"
          "along with this program; if not, write to the Free Software\n"
          "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n\n"

          "CREDITS:\n"
          "  Based on acdcontrol, written by Pavel Gurevich.\n\n"
          );
}

////////////////////////////////////////////////////////////////////////////////
//                      _
//                     (_)
//  _ __ ___     __ _   _    _ __
// | '_ ` _ \   / _` | | |  | '_ \
// | | | | | | | (_| | | |  | | | |
// |_| |_| |_|  \__,_| |_|  |_| |_|
//
////////////////////////////////////////////////////////////////////////////////
int main (int argc, char **argv) {
  int fd = -1;
  int rd, i;
  int alv, yalv;
  struct hiddev_devinfo device_info;
  struct hiddev_report_info rep_info;
  struct hiddev_field_info field_info;
  struct hiddev_usage_ref usage_ref;
  struct hiddev_event ev[64];
  fd_set fdset;
  int report_type;
  int appl;
  int version;
  int brightness = 0;
  int amount = 0;
  int mode = USAGE_MODE_GET;
  int open_mode = O_RDONLY;
  
  /* Behavior options */
  bool brief  = false;
  bool silent = false;
  bool force = false;

  bool first_device=true;
    
  int c;
  int digit_optind = 0;

  const DeviceId* selected_device = 0;

  init_device_database();
  
  while (1) {
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
      {"about", 0, 0, 'a'},
      {"brief", 0, 0, 'b'},
      {"help", 0, 0, 'h'},
      {"silent", 0, 0, 's'},
      {"force", 0, 0, 'f'},
      {"detect", 0, 0, 'd'},
      {"list-all", 0, 0, 'l'},
      {0, 0, 0, 0}
    };
      
    c = getopt_long (argc, argv, "abhsdl",
                     long_options, &option_index);
    if (c == -1)
      break;
      
    switch (c) {
    case 'a':
      about();
      exit( 0 );
        
    case 'b':
      brief=true;
      break;
        
    case 'h':
      help( argv[0] );
      exit( 0 );
      break;
        
    case 's':
      silent=true;
      break;
      
    case 'f':
      force=true;
      break;

    case 'd':
      mode=USAGE_MODE_DETECT;
      break;

    case 'l':
      dump_supported();
      exit( 0 );
        
    default:
      fprintf (stderr,"Unknown option '%c'\n", c);
      help( argv[0] );
      exit( 2 );
    }
  }

  typedef list< const char* > FileList;
  FileList files;
  
  for ( int param = optind; param < argc; ++param ) {
    if ( mode != USAGE_MODE_DETECT && number ( argv[ param ] ) ) {
      if ( argv[ param ][0] == '+' || argv[ param ][0] == '-' ) {
        mode = USAGE_MODE_SETREL;
        amount = atoi ( argv[ param ] );
      } else {
        mode = USAGE_MODE_SET;
        brightness = atoi ( argv[ param ] );
      }
      continue;
    }

    files.push_back( argv[ param ] );
  }

  if ( files.empty() ) {
    help( argv[0] );
    exit( 1 );
  }

  if ( mode == USAGE_MODE_SET || mode == USAGE_MODE_SETREL ) {
    open_mode = O_RDWR;
  }

  if ( !silent )
    notice();

  for ( FileList::iterator it = files.begin(); it != files.end();
        ++it ) {
    if (( fd = open( *it, open_mode )) < 0) {
      perror(*it);
      continue;
    }
    
    /* ioctl() accesses the underlying driver */
    ioctl(fd, HIDIOCGVERSION, &version);
    /* the HIDIOCGVERSION ioctl() returns a packed 32 field (aka integer) */
    /* so we unpack it and display it */
    if ( ! silent && first_device )
      printf("hiddev driver version is %d.%d.%d\n",
             version >> 16, (version >> 8) & 0xff, version & 0xff);
    
    /* suck out some device information */
    ioctl(fd, HIDIOCGDEVINFO, &device_info);
    
    if ( mode == USAGE_MODE_DETECT ) {
      if ( is_usb_monitor( device_info, fd ) ) {
        cout << *it << ": USB Monitor - "
             << (is_supported( device_info ) ? "SUPPORTED": "UNSUPPORTED")
             << ".\t";
        format_device( cout, device_info );
      }
      continue;
    }

    if ( not (selected_device = is_supported ( device_info )) ){
      cerr << "Device unsupported:";
      format_device(cerr, device_info);
      if ( !force )
        exit ( 2 );
    }
    
    
    if (! is_usb_monitor( device_info, fd )) {
      cerr << *it << ": This device is NOT USB monitor!" << endl;
      continue;
    }
    
    /* Initialise the internal report structures */
    if (ioctl(fd, HIDIOCINITREPORT,0) < 0) {
      cerr << "FATAL: Failed to initialize internal report structures"
           << endl;
      exit(1);
    }
    
    usage_ref.report_type = HID_REPORT_TYPE_FEATURE;
    usage_ref.report_id = BRIGHTNESS_CONTROL;
    usage_ref.field_index = 0;
    usage_ref.usage_index = 0;
    usage_ref.usage_code = USAGE_CODE;
    usage_ref.value = brightness;
    //  dump_usage ( usage_ref );
    
    rep_info.report_type = HID_REPORT_TYPE_FEATURE;
    rep_info.report_id = BRIGHTNESS_CONTROL;
    rep_info.num_fields = 1;

    if ( mode == USAGE_MODE_SET ) {
      if ( ioctl(fd, HIDIOCSUSAGE, &usage_ref) < 0 ) {
        perror ("Usage failed!");
        exit ( 2 );
      }
      if ( ioctl(fd, HIDIOCSREPORT, &rep_info) < 0 ) {
        perror ("Report failed!");
        exit ( 3 );
      }
    } else {
      if ( ioctl(fd, HIDIOCGUSAGE, &usage_ref) < 0 ) {
        perror ("Usage failed!");
        exit ( 2 );
      }
      if ( ioctl(fd, HIDIOCGREPORT, &rep_info) < 0 ) {
        perror ("Report failed!");
        exit ( 3 );
      }
      if ( mode == USAGE_MODE_SETREL ) {
        brightness = usage_ref.value + amount;
        brightness = max( selected_device->brightness_min, brightness);
        brightness = min( selected_device->brightness_max, brightness);
        usage_ref.value = brightness;

        /* set calculated brightness */
        if ( ioctl(fd, HIDIOCSUSAGE, &usage_ref) < 0 ) {
          perror ("Usage failed!");
          exit ( 2 );
        }
        if ( ioctl(fd, HIDIOCSREPORT, &rep_info) < 0 ) {
          perror ("Report failed!");
          exit ( 3 );
        }
        /* read brightness back from device */
        if ( ioctl(fd, HIDIOCGUSAGE, &usage_ref) < 0 ) {
          perror ("Usage failed!");
          exit ( 2 );
        }
        if ( ioctl(fd, HIDIOCGREPORT, &rep_info) < 0 ) {
          perror ("Report failed!");
          exit ( 3 );
        }
      }
      if ( !brief )
        cout << *it << ": BRIGHTNESS=";
      cout << usage_ref.value << endl;
    }

    close(fd);
    first_device=false;
  }
}


void init_device_database() {
  supportedVendors.insert( VendorDesc( APPLE, "Apple" ) );

  supportedDevices.insert( DeviceId( APPLE, STUDIO_DISPLAY_27,
                                     "Apple Studio Display 27\"", 400, 60000 ));
}

void dump_supported () {
  for ( SupportedDevices::iterator it = supportedDevices.begin();
        it != supportedDevices.end(); ++ it )
    cout << "Vendor=" << setw( 6 ) << hex << showbase << it->vendor
         << " (" << supportedVendors[ it->vendor ] << "), "
         << "Product=" << it->product << " [" 
         << it->description << "]" << endl;
}

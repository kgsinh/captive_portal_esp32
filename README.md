# ESP32 Captive Portal with RFID Management

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.0+-blue.svg)](https://github.com/espressif/esp-idf)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ESP32-orange.svg)](https://www.espressif.com/en/products/socs/esp32)

A feature-rich ESP32-based captive portal system with advanced WiFi management, RFID card database, OTA updates, and web-based configuration interface.

## 📋 Table of Contents

- [Features](#-features)
- [Architecture](#-architecture)
- [Hardware Requirements](#-hardware-requirements)
- [Getting Started](#-getting-started)
- [Configuration](#-configuration)
- [Usage](#-usage)
- [API Documentation](#-api-documentation)
- [Components](#-components)
- [Development](#-development)
- [Troubleshooting](#-troubleshooting)
- [License](#-license)

## ✨ Features

### Core Functionality
- **Dual WiFi Mode**: Simultaneous Access Point (AP) and Station (STA) operation
- **Captive Portal**: Automatic DNS redirection triggering sign-in popups on Android, iOS, and Windows
- **RFID Card Management**: Complete database system for managing authorized RFID cards
- **OTA Updates**: Wireless firmware updates via web interface
- **Persistent Storage**: NVS-based WiFi credential storage and SPIFFS-based RFID database
- **Time Synchronization**: NTP-based local and UTC time management
- **Web Interface**: Responsive HTML/CSS/JS interface with jQuery

### Advanced Features
- **Protected Admin Cards**: Prevent deletion of critical RFID cards
- **RESTful API**: Comprehensive HTTP endpoints for all operations
- **Thread-Safe Operations**: Mutex-protected RFID database access
- **Database Integrity**: Checksum validation and file size verification
- **Diagnostic Tools**: WiFi network scanning with detailed logging
- **Custom Partitions**: Flexible NVS partition management

## 🏗 Architecture

The system follows a modular architecture with independent components:

```
┌─────────────────────────────────────────────────────────────┐
│                         Main Application                     │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   WiFi AP    │  │  WiFi STA    │  │  HTTP Server │      │
│  │  (Captive)   │  │  (Client)    │  │  (Port 80)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│         │                  │                  │              │
│         └──────────────────┼──────────────────┘              │
│                            │                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ DNS Server   │  │     RFID     │  │  OTA Update  │      │
│  │  (Port 53)   │  │   Manager    │  │    Handler   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│         │                  │                  │              │
│         └──────────────────┼──────────────────┘              │
│                            │                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ NVS Storage  │  │    SPIFFS    │  │  Time Sync   │      │
│  │  (WiFi Creds)│  │ (RFID DB)    │  │     (NTP)    │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

### Data Flow

1. **Initial Connection**: Device creates AP → DNS server redirects all requests → HTTP server serves captive portal
2. **WiFi Configuration**: User submits credentials → Stored in NVS → ESP32 connects as Station
3. **RFID Management**: Web interface → RESTful API → RFID Manager → SPIFFS database
4. **OTA Updates**: Firmware upload → Validation → Flash update → Auto-restart

## 🔧 Hardware Requirements

### Supported Platforms
| Chip | Status |
|------|--------|
| ESP32 | ✅ Supported |
| ESP32-S2 | ✅ Supported |
| ESP32-S3 | ✅ Supported |
| ESP32-C3 | ✅ Supported |
| ESP32-C6 | ✅ Supported |
| ESP32-C2 | ✅ Supported |

### Minimum Requirements
- **Flash**: 4MB (partition table configured for 4MB)
- **RAM**: Standard ESP32 memory
- **USB Cable**: For power supply and programming
- **WiFi**: Built-in WiFi interface

### Optional Hardware
- RFID reader module (for physical RFID card reading)
- External antenna (for improved range)

## 🚀 Getting Started

### Prerequisites

1. **ESP-IDF v5.0 or later**
   ```bash
   # Install ESP-IDF
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   ./install.sh
   . ./export.sh
   ```

2. **Build Tools**
   - CMake 3.16 or later
   - Python 3.8 or later
   - Git

### Quick Start

1. **Clone the Repository**
   ```bash
   git clone <repository-url>
   cd captive_portal_esp32
   ```

2. **Set Target Chip**
   ```bash
   idf.py set-target esp32
   # or esp32s2, esp32s3, esp32c3, esp32c6, esp32c2
   ```

3. **Configure the Project**
   ```bash
   idf.py menuconfig
   ```
   
   Navigate to `Example Configuration` and set:
   - **SoftAP SSID**: Your Access Point name (default: `esp32_ssid`)
   - **SoftAP Password**: AP password (default: `esp32_pwd`)
   - **Maximal STA Connections**: Max clients (default: 4)
   - **WiFi SSID**: Target network for STA mode
   - **WiFi Password**: Password for STA mode

4. **Build and Flash**
   ```bash
   idf.py build
   idf.py -p PORT flash monitor
   ```
   
   Replace `PORT` with your serial port (e.g., `/dev/ttyUSB0` on Linux, `COM3` on Windows)

5. **Exit Monitor**
   Press `Ctrl+]` to exit the serial monitor

### First-Time Setup

1. **Connect to the Access Point**
   - Look for WiFi network named `esp32_ssid` (or your configured SSID)
   - Connect with password `esp32_pwd` (or your configured password)

2. **Automatic Portal**
   - Captive portal popup should appear automatically
   - If not, navigate to `192.168.4.1` in a browser

3. **Configure WiFi**
   - Use the web interface to configure Station mode WiFi credentials
   - Device will connect to your network while maintaining the AP

## ⚙ Configuration

### WiFi Configuration

#### Access Point (AP) Settings
Configure via `idf.py menuconfig` → `Example Configuration`:

| Option | Default | Description |
|--------|---------|-------------|
| `CONFIG_ESP_WIFI_AP_SSID` | `esp32_ssid` | Access Point network name |
| `CONFIG_ESP_WIFI_AP_PASSWORD` | `esp32_pwd` | AP password (8-63 chars) |
| `CONFIG_ESP_MAX_AP_STA_CONN` | `4` | Maximum concurrent connections |

#### Station (STA) Settings
Configure via menuconfig or web interface:

| Option | Default | Description |
|--------|---------|-------------|
| `CONFIG_ESP_WIFI_SSID` | `` | Target WiFi network |
| `CONFIG_ESP_WIFI_PASSWORD` | `` | WiFi password |
| `CONFIG_ESP_MAXIMUM_RETRY` | `5` | Connection retry attempts |

### RFID Default Cards

The system includes three default RFID cards:

| Card ID | Name | Protected |
|---------|------|-----------|
| `0x12345678` | Admin Card | Yes (cannot be deleted) |
| `0x87654321` | User Card 1 | No |
| `0xABCDEF00` | User Card 2 | No |

### Partition Table

Using custom partition table (`partition-rev-1-4mb.csv`):
- **NVS**: 24KB
- **PHY_INIT**: 4KB  
- **Factory App**: ~1.5MB
- **OTA_0**: ~1.3MB
- **OTA_1**: ~1.3MB
- **SPIFFS**: Remaining space for RFID database

### Build Configuration

Key options in `sdkconfig.defaults`:
```ini
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partition-rev-1-4mb.csv"
```

## 📖 Usage

### Web Interface

#### Main Configuration Page
Access at `http://192.168.4.1/` or captive portal

Features:
- WiFi credential configuration
- System information display
- OTA firmware update
- Real-time sensor data (if connected)
- Time synchronization status

#### RFID Management Interface
Access at `http://192.168.4.1/rfid.html`

Features:
- View all registered RFID cards
- Add new cards with custom names
- Remove cards (except protected admin card)
- Reset database to defaults
- Check card authorization status

### WiFi Management

1. **View Current Configuration**
   - Main page displays STA connection status
   - Shows connected network SSID and IP

2. **Change WiFi Network**
   - Enter new SSID and password
   - Submit via web interface
   - Credentials stored in NVS
   - Device reconnects automatically

3. **Network Diagnostics**
   - System performs automatic WiFi scan on boot
   - Logs available networks with RSSI and security info
   - Helps troubleshoot connection issues

### RFID Card Management

1. **Add New Card**
   ```
   POST /cards/add
   {
     "id": 1234567890,
     "nm": "New Card Name"
   }
   ```

2. **Remove Card**
   ```
   DELETE /cards/remove?id=1234567890
   ```

3. **Check Card**
   ```
   GET /cards/check
   {
     "card_id": "1234567890"
   }
   ```

4. **List All Cards**
   ```
   GET /cards/get
   ```

5. **Reset to Defaults**
   ```
   POST /cards/reset
   ```

### OTA Firmware Update

1. **Prepare Firmware**
   - Build project: `idf.py build`
   - Locate binary: `build/captive_portal_esp32.bin`

2. **Upload via Web Interface**
   - Navigate to main page
   - Select firmware file
   - Click upload
   - Wait for completion (device restarts automatically)

3. **Monitor Progress**
   - Serial monitor shows OTA progress
   - Web interface displays update status
   - Device reboots on success

## 📡 API Documentation

### HTTP Endpoints

#### Static Resources
| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Main HTML page |
| `/rfid.html` or `/rfid` | GET | RFID management page |
| `/app.css` | GET | Main stylesheet |
| `/app.js` | GET | Main JavaScript |
| `/rfid.css` | GET | RFID stylesheet |
| `/rfid.js` | GET | RFID JavaScript |
| `/jquery-3.3.1.min.js` | GET | jQuery library |
| `/favicon.ico` | GET | Favicon |

#### System Information
| Endpoint | Method | Response | Description |
|----------|--------|----------|-------------|
| `/apSSID` | GET | `{"ssid":"..."}` | Get AP SSID |
| `/localTime` | GET | `{"local_time":"...", "utc_time":"..."}` | Get system time |
| `/Sensor` | GET | `{"temp":25, "humidity":60}` | Get sensor data |
| `/OTAstatus` | POST | `{"ota_update_status":0, "compile_time":"...", "compile_date":"..."}` | OTA status |

#### WiFi Management
| Endpoint | Method | Headers | Response | Description |
|----------|--------|---------|----------|-------------|
| `/wifiConnect` | POST | `my-connect-ssid`, `my-connect-pswd` | `{"ssid":"...", "password":"..."}` | Set WiFi credentials |

#### RFID Management
| Endpoint | Method | Body/Params | Response | Description |
|----------|--------|-------------|----------|-------------|
| `/cards/get` | GET | - | `{"status":"ok", "count":N, "cards":[...]}` | List all cards |
| `/cards/defaults` | GET | - | `{"status":"ok", "count":3, "cards":[...]}` | Get default cards |
| `/cards/add` | POST | `{"id":123, "nm":"Name"}` | `{"status":"success"}` | Add new card |
| `/cards/remove` | DELETE | `?id=123` | `{"status":"success"}` | Remove card |
| `/cards/count` | GET | - | `{"card_count":N}` | Get card count |
| `/cards/check` | GET | `{"card_id":"123"}` | `{"exists":true/false}` | Check if card exists |
| `/cards/reset` | POST | - | `{"status":"success"}` | Reset to defaults |

#### OTA Updates
| Endpoint | Method | Body | Response | Description |
|----------|--------|------|----------|-------------|
| `/OTAupdate` | POST | Binary firmware file | - | Upload firmware |

#### Generic Data Query
| Endpoint | Method | Body | Response | Description |
|----------|--------|------|----------|-------------|
| `/getData` | POST | `{"key":"SSID,Temp,Humidity"}` | `{"SSID":"...", "Temp":"...", "Humidity":"..."}` | Query multiple values |

### Response Codes

| Code | Meaning |
|------|---------|
| 200 | Success |
| 201 | Created (card added) |
| 400 | Bad Request (invalid parameters) |
| 403 | Forbidden (e.g., trying to delete admin card) |
| 404 | Not Found (card not found) |
| 500 | Internal Server Error |

### Error Handling

All API endpoints return structured JSON errors:
```json
{
  "status": "error",
  "message": "Description of the error"
}
```

## 🧩 Components

### app_wifi
**Purpose**: WiFi management for both AP and STA modes

**Key Functions**:
- `sta_wifi_init()`: Initialize Station mode
- `softap_init()`: Initialize Access Point
- `wifi_scan_networks()`: Scan and log available networks

**Features**:
- Automatic retry on connection failure
- Detailed disconnect reason logging
- Network diagnostics

### app_local_server
**Purpose**: HTTP server and request handling

**Key Functions**:
- `app_local_server_init()`: Initialize server
- `app_local_server_start()`: Start HTTP and DNS servers
- `app_local_server_process()`: Process server events

**Features**:
- 20+ HTTP endpoints
- Thread-safe message queue
- OTA update handling
- Embedded static files

### dns_server
**Purpose**: DNS redirection for captive portal

**Key Functions**:
- `start_dns_server()`: Start DNS server task
- `parse_dns_request()`: Parse and respond to DNS queries

**Features**:
- Redirects all DNS A-type queries to AP IP
- Handles multiple concurrent requests
- Automatic socket recovery

### rfid_manager
**Purpose**: RFID card database management

**Key Functions**:
- `rfid_manager_init()`: Initialize database
- `rfid_manager_add_card()`: Add new card
- `rfid_manager_remove_card()`: Remove card (with protection)
- `rfid_manager_check_card()`: Verify card authorization
- `rfid_manager_get_card_list_json()`: Export cards as JSON

**Features**:
- Mutex-protected thread-safe operations
- Admin card protection
- Database integrity validation
- File size verification
- Checksum support
- Default card loading

### nvs_storage
**Purpose**: Non-volatile storage for WiFi credentials

**Key Functions**:
- `nvs_storage_init()`: Initialize NVS
- `nvs_storage_set_wifi_credentials()`: Store credentials
- `nvs_storage_get_wifi_credentials()`: Retrieve credentials

**Features**:
- Automatic partition recovery
- Error handling and retry logic

### spiffs_storage
**Purpose**: File system operations for RFID database

**Key Functions**:
- `spiffs_storage_init()`: Mount SPIFFS
- `spiffs_storage_write_file()`: Write to file
- `spiffs_storage_read_file()`: Read from file
- `spiffs_storage_file_exists()`: Check file existence
- `spiffs_storage_delete_file()`: Delete file

**Features**:
- Append and overwrite modes
- File size queries
- Automatic directory creation

### app_time_sync
**Purpose**: Network time synchronization

**Key Functions**:
- `time_sync_init()`: Initialize NTP client
- Time zone configuration
- UTC and local time management

### custom_partition
**Purpose**: Custom NVS partition management

**Features**:
- Additional NVS partition support
- Flexible storage allocation

## 💻 Development

### Project Structure
```
captive_portal_esp32/
├── main/
│   ├── main.c                 # Application entry point
│   └── Kconfig.projbuild      # Configuration options
├── components/
│   ├── app_wifi/              # WiFi management
│   ├── app_local_server/      # HTTP server & DNS
│   │   └── webpage/           # Embedded web files
│   ├── rfid_manager/          # RFID database
│   │   └── test/              # Unit tests
│   ├── nvs_storage/           # NVS operations
│   ├── spiffs_storage/        # SPIFFS operations
│   │   └── test/              # Unit tests
│   ├── app_time_sync/         # Time synchronization
│   └── custom_partition/      # Custom partitions
├── test/                      # Integration tests
├── CMakeLists.txt             # Build configuration
├── sdkconfig.defaults         # Default configuration
└── partition-rev-1-4mb.csv    # Partition table
```

### Building for Different Targets

```bash
# ESP32
idf.py set-target esp32
idf.py build

# ESP32-S3
idf.py set-target esp32s3
idf.py build

# ESP32-C3
idf.py set-target esp32c3
idf.py build
```

### Adding New Features

1. **New Component**
   ```bash
   mkdir components/my_component
   # Create CMakeLists.txt and source files
   ```

2. **New HTTP Endpoint**
   - Add handler function in `app_local_server.c`
   - Add to `uri_handlers[]` array
   - Update `HTTP_SERVER_MAX_URI_HANDLERS` if needed

3. **New Web Page**
   - Add HTML/CSS/JS to `components/app_local_server/webpage/`
   - Update `CMakeLists.txt` to embed files
   - Add handler function

### Testing

Run unit tests:
```bash
cd test
idf.py build
idf.py -p PORT flash monitor
```

### Debugging

1. **Enable Debug Logs**
   ```bash
   idf.py menuconfig
   # Component config → Log output → Default log verbosity → Debug
   ```

2. **Monitor Serial Output**
   ```bash
   idf.py monitor
   ```

3. **GDB Debugging**
   ```bash
   idf.py gdb
   ```

### Code Style

Following ESP-IDF and embedded C best practices:
- `snake_case` for functions and variables
- `UPPER_SNAKE_CASE` for macros and constants
- Doxygen-style comments for public APIs
- Mutex protection for shared resources
- Error checking with `ESP_ERROR_CHECK` or `ESP_GOTO_ON_ERROR`

## 🔍 Troubleshooting

### Connection Issues

**Problem**: Cannot connect to Access Point
- **Solution**: Check SSID and password in menuconfig
- **Solution**: Verify device is in range
- **Solution**: Check maximum STA connections limit

**Problem**: Captive portal doesn't appear
- **Solution**: Manually navigate to `192.168.4.1`
- **Solution**: Disable VPN if active
- **Solution**: Check DNS server logs in serial monitor

**Problem**: Station mode won't connect
- **Solution**: Verify WiFi credentials are correct
- **Solution**: Check network security type compatibility
- **Solution**: Review WiFi scan results in logs
- **Solution**: Ensure network is 2.4GHz (ESP32 doesn't support 5GHz)

### RFID Database Issues

**Problem**: Cannot add cards
- **Solution**: Check SPIFFS is initialized (logs)
- **Solution**: Verify database isn't full (max 200 cards)
- **Solution**: Check card ID isn't duplicate

**Problem**: Database corrupted
- **Solution**: Use `/cards/reset` endpoint to reset to defaults
- **Solution**: Re-flash with partition erase: `idf.py -p PORT erase-flash flash`

**Problem**: Admin card deleted accidentally
- **Solution**: Reset database to restore default cards
- **Note**: Admin card (0x12345678) should be protected from deletion

### OTA Update Issues

**Problem**: OTA update fails
- **Solution**: Verify firmware file is correct (`.bin` file from build)
- **Solution**: Check available flash space
- **Solution**: Ensure stable power supply during update
- **Solution**: Review OTA logs in serial monitor

**Problem**: Device doesn't restart after OTA
- **Solution**: Manually reset the device
- **Solution**: Check serial logs for errors

### Memory Issues

**Problem**: Crashes or resets
- **Solution**: Enable core dump: `idf.py menuconfig → Core dump`
- **Solution**: Check stack sizes in component configs
- **Solution**: Monitor heap with `esp_get_free_heap_size()`

### Build Issues

**Problem**: Build fails
- **Solution**: Clean build: `idf.py fullclean`
- **Solution**: Update ESP-IDF: `git pull && git submodule update --recursive`
- **Solution**: Check ESP-IDF version compatibility (v5.0+)

### Common Log Messages

| Log Message | Meaning | Action |
|-------------|---------|--------|
| `RFID database is not valid` | Database file corrupted | Reset database |
| `WiFi disconnected, reason: X` | WiFi connection lost | Check credentials and network |
| `Failed to allocate memory` | Heap exhausted | Reduce buffer sizes or restart |
| `Socket unable to bind: errno X` | Port already in use | Restart device |

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 📞 Support

For issues and questions:
- Open an issue on GitHub
- Review the troubleshooting section
- Check ESP-IDF documentation

## 🙏 Acknowledgments

- Based on ESP-IDF captive portal example
- Built with ESP-IDF framework by Espressif Systems
- Uses jQuery for web interface functionality

---

**Note**: This project demonstrates advanced ESP-IDF features including dual WiFi modes, persistent storage, OTA updates, and embedded web servers. It's suitable for IoT applications requiring WiFi configuration, access control, and remote management.

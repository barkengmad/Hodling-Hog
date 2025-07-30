# Hodling Hog 🐷⚡

**Saving your future, one oink at a time!**

A minimalist, low-power Bitcoin piggy bank built on ESP32 with e-paper display. Monitor your Lightning wallet and cold storage balances, receive payments via QR codes, and manage your stack with a simple, elegant interface.

![Hodling Hog Preview](assets/hodling-hog-preview.png)

## 🚀 Features

### Core Functionality
- **Lightning Balance Display**: Real-time balance from custodial wallets (Wallet of Satoshi)
- **Cold Storage Monitoring**: Track your on-chain Bitcoin addresses
- **QR Code Generation**: Receive payments directly to your wallets
- **Screen Cycling**: Switch between Lightning, cold storage, and combined views
- **Low Power Design**: E-paper display with deep sleep modes
- **Wake-on-Tilt**: Shake to wake and update balances

### Interaction
- **Single Button Interface**: Short press to cycle screens, long press for config
- **Tilt Switch**: Wake device and trigger balance updates
- **Web Configuration**: Local captive portal for easy setup
- **Auto Sleep**: Power-saving sleep mode when inactive

### Connectivity
- **Non-blocking WiFi**: Robust connection handling with offline fallback
- **API Integration**: Wallet of Satoshi and Blockstream.info APIs
- **NTP Sync**: Automatic time synchronization
- **mDNS Discovery**: Easy network discovery

### Advanced Features
- **Transaction Management**: View history and create transfers
- **Signing Support**: Optional transaction signing for cold storage
- **Configuration Backup**: Export/import settings via QR codes
- **Persistent Storage**: LittleFS-based configuration management

## 🛠 Hardware Requirements

### Core Components
- **ESP32 DevKit v1** (ESP-WROOM-32)
- **1.54" E-paper Display** (200x200, 8-pin SPI)
- **Tactile Push Button**
- **Tilt Switch**
- **Power Supply** (LiPo battery + TP4056 charging module)

### Pin Mapping

| Component | Signal | ESP32 Pin | Notes |
|-----------|--------|-----------|-------|
| E-paper | VCC | 3.3V | Power |
| E-paper | GND | GND | Ground |
| E-paper | DIN (MOSI) | GPIO23 | SPI Data |
| E-paper | CLK (SCK) | GPIO18 | SPI Clock |
| E-paper | CS | GPIO5 | Chip Select |
| E-paper | DC | GPIO17 | Data/Command |
| E-paper | RST | GPIO16 | Reset |
| E-paper | BUSY | GPIO4 | Busy Status |
| Button | Input | GPIO0 | Boot mode compatible |
| Tilt Switch | Input | GPIO2 | Wake trigger |

### Optional Components
- **3.3V Regulator** (if using non-3.3V battery)
- **LED Indicator** (status/debugging)
- **Enclosure** (weatherproof recommended)

## 📦 Installation

### Prerequisites
1. **PlatformIO IDE** (VS Code extension recommended)
2. **Git** for cloning the repository
3. **ESP32 Board Package** (automatically installed by PlatformIO)

### Setup Steps

1. **Clone the Repository**
   ```bash
   git clone https://github.com/yourusername/hodling-hog.git
   cd hodling-hog
   ```

2. **Configure Secrets**
   ```bash
   cp src/secrets.h.example src/secrets.h
   ```
   
   Edit `src/secrets.h` with your credentials:
   ```cpp
   #define WIFI_SSID "your_wifi_network"
   #define WIFI_PASSWORD "your_wifi_password"
   #define WOS_API_TOKEN "your_wallet_of_satoshi_token"
   #define COLD_STORAGE_ADDRESS "your_bitcoin_address"
   ```

3. **Install Dependencies**
   ```bash
   pio lib install
   ```

4. **Build and Upload**
   ```bash
   pio run --target upload
   ```

5. **Monitor Serial Output**
   ```bash
   pio device monitor
   ```

## ⚙️ Configuration

### Initial Setup

1. **Power on the device** - it will show the boot screen
2. **Connect to WiFi** or enter configuration mode:
   - **Automatic**: Device connects using `secrets.h` credentials
   - **Manual**: Hold button during boot to enter config mode

### Configuration Mode

When in config mode, the device creates a WiFi access point:
- **SSID**: `HodlingHog-Config`
- **Password**: `hodling123`
- **Web Interface**: `http://192.168.4.1`

### Web Interface Sections

#### WiFi Settings
- Network credentials
- Connection timeout
- Hostname configuration

#### Lightning Wallet
- Wallet of Satoshi API token
- Update intervals
- Transfer settings

#### Cold Storage
- Bitcoin address monitoring
- Private key (optional, for signing)
- API endpoint selection

#### Display
- Brightness control
- Screen timeout
- Default view selection

#### Power Management
- Sleep timeout
- Wake options
- Battery monitoring

## 🔧 Usage

### Basic Operation

#### Screen Navigation
- **Short Press**: Cycle between screens
  1. Lightning balance + QR
  2. Cold storage balance + QR  
  3. Combined balance view
- **Long Press**: Show device IP/config URL
- **Double Click**: Force balance update

#### Tilt Functionality
- **Gentle Tilt**: Wake device from sleep
- **Shake**: Wake + immediate balance update

#### Sleep Behavior
- **Auto Sleep**: After 1 minute of inactivity
- **Deep Sleep**: Minimal power consumption
- **Wake Sources**: Button press or tilt activation

### Advanced Features

#### Transaction Transfers
1. Access web interface
2. Navigate to "Transfer" section
3. Specify amount and destination
4. Confirm transaction (signing if enabled)

#### Configuration Backup
1. Go to "System" in web interface
2. Click "Export Configuration"
3. Save QR code or JSON file
4. Import on new device via "Import Configuration"

#### Firmware Updates
1. Build new firmware: `pio run`
2. Access web interface
3. Navigate to "System" → "Firmware Update"
4. Upload `.bin` file

## 🔐 Security

### Best Practices
- **Wallet of Satoshi**: Use a dedicated wallet with limited funds
- **Cold Storage**: Watch-only addresses recommended
- **Private Keys**: Only store if absolutely necessary, encrypted at rest
- **Network**: Use WPA3 WiFi networks when possible
- **Physical**: Secure device mounting, tamper-evident case

### Privacy Considerations
- **API Calls**: Balances fetched over HTTPS
- **Local Storage**: Configuration encrypted on device
- **Network**: No telemetry or external logging
- **Open Source**: Fully auditable code

## 🛡️ Troubleshooting

### Common Issues

#### WiFi Connection Problems
```
- Check SSID/password in secrets.h
- Verify 2.4GHz network (ESP32 limitation)  
- Try manual config mode (hold button at boot)
- Check signal strength and interference
```

#### Display Issues
```
- Verify SPI pin connections
- Check power supply (3.3V requirement)
- Ensure proper grounding
- Try different display update modes
```

#### API Errors
```
- Validate API tokens in secrets.h
- Check internet connectivity
- Verify API service status
- Review serial monitor for error details
```

#### Power/Sleep Issues
```
- Check wake pin connections (GPIO0, GPIO2)
- Verify battery voltage levels
- Review sleep timeout settings
- Monitor for memory leaks
```

### Debug Mode
Enable verbose logging:
```cpp
// In main.cpp
utils.enableDebug(true);
```

Monitor serial output:
```bash
pio device monitor --baud 115200
```

## 🔮 Future Roadmap

### Planned Features
- [ ] **Lightning Invoice Generation**: Create custom payment requests
- [ ] **Multiple Wallet Support**: Support additional Lightning providers
- [ ] **Mempool Integration**: Transaction fee estimation
- [ ] **Price Feeds**: Fiat conversion rates
- [ ] **Tor Support**: Privacy-enhanced connectivity
- [ ] **Hardware Wallet Integration**: Trezor/Ledger support
- [ ] **Multi-signature**: Advanced cold storage setups
- [ ] **Mobile App**: Companion smartphone application

### Hardware Improvements
- [ ] **Larger Display**: 2.9" or 4.2" e-paper options
- [ ] **Solar Charging**: Self-sustaining power
- [ ] **Environmental Sensors**: Temperature, humidity
- [ ] **Sound**: Simple beeper for notifications
- [ ] **RGB LED**: Status indication improvements

## 🤝 Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Setup
1. Fork the repository
2. Create feature branch: `git checkout -b feature/amazing-feature`
3. Make changes and test thoroughly
4. Submit pull request with clear description

### Areas for Contribution
- **Hardware**: New board designs and components
- **Software**: Additional wallet integrations
- **UI/UX**: Display layout and interaction improvements
- **Documentation**: Tutorials, translations, guides
- **Testing**: Unit tests, integration tests, hardware validation

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Satoshi Nakamoto** - For creating Bitcoin
- **Lightning Network Developers** - For scaling Bitcoin
- **PlatformIO Team** - For excellent embedded development tools
- **ESP32 Community** - For hardware and software support
- **E-paper Display Libraries** - For display functionality

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/hodling-hog/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/hodling-hog/discussions)
- **Documentation**: [Wiki](https://github.com/yourusername/hodling-hog/wiki)

---

**Disclaimer**: This is an experimental project. Use at your own risk. Always verify transactions and maintain proper backups of your Bitcoin keys and configuration.

*Hodling Hog - Because the best time to save Bitcoin was 10 years ago, the second best time is now! 🐷⚡* 
#include <LovyanGFX.hpp>

#define SPI_HOST_ID   SPI2_HOST
#define SPI_FREQUENCY 40000000

/*--------------------------- SETUP LGFX ----------------------------------*/

class LGFX : public lgfx::LGFX_Device
{
    // provide an instance that matches the type of panel you want to connect to
    lgfx::Panel_ST7789 _panel_instance;
    // provide an instance that matches the type of bus to which the panel is connected.
    lgfx::Bus_SPI _bus_instance; // Instances of spi buses

    lgfx::Light_PWM _light_instance;

public:
  LGFX(void)
  {
    {
      // set up bus control.
      auto cfg = _bus_instance.config(); // gets the structure for bus settings.

      // SPI bus settings
      cfg.spi_host = SPI_HOST_ID; // Select SPI to use ESP32-S2,C3: SPI2_HOST or SPI3_HOST / ESP32: VSPI_HOST or HSPI_HOST

      //* Due to the ESP-IDF upgrade, the description of VSPI_HOST , HSPI_HOST will be deprecated, so if you get an error, use SPI2_HOST , SPI3_HOST instead.
      cfg.spi_mode = 0;               // Set SPI communication mode (0-3)
      cfg.freq_write = SPI_FREQUENCY; // SPI clock on transmission (up to 80MHz, rounded to 80MHz divided by integer)
      cfg.freq_read = 16000000;       // SPI clock on reception
      cfg.spi_3wire = true;           // Set true when receiving on the MOSI pin
      cfg.use_lock = true;            // set true if transaction lock is used

      //  * With the ESP-IDF version upgrade, SPI_DMA_CH_AUTO (automatic setting) of DMA channels is recommended.
      cfg.dma_channel = SPI_DMA_CH_AUTO; // Set DMA channel to be used (0=DMA not used / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=Auto setting)

      cfg.pin_sclk = TFT_SCLK; // Set SCLK pin number for SPI
      cfg.pin_mosi = TFT_MOSI; // Set SPI MOSI pin number

      // When using the SPI bus, which is common to the SD card, be sure to set MISO without omitting it.
      cfg.pin_miso = TFT_MISO; // Set THE MSO pin number of spi (-1 = disable)
      cfg.pin_dc = TFT_DC;     // Set THE D/C pin number of SPI (-1 = disable)

      _bus_instance.config(cfg);              // reflects the setting value on the bus.
      _panel_instance.setBus(&_bus_instance); // Set the bus to the panel.
    }

    {
      // set the display panel control.
      auto cfg = _panel_instance.config(); // gets the structure for display panel settings.

      cfg.pin_cs = TFT_CS;   // Pin number to which CS is connected (-1 = disable)
      cfg.pin_rst = TFT_RST; // Pin number to which RST is connected (-1 = disable)
      cfg.pin_busy = -1;     // Pin number to which BUSY is connected (-1 = disable)

      // the following setting values are set to a general initial value for each panel,
      cfg.panel_width = TFT_WIDTH;   // actual visible width
      cfg.panel_height = TFT_HEIGHT; // actually visible height
      cfg.offset_x = 0;              // Panel X-direction offset amount
      cfg.offset_y = 0;              // Panel Y offset amount
      cfg.offset_rotation = 0;       // offset of rotational values from 0 to 7 (4 to 7 upside down)
      cfg.dummy_read_pixel = 8;      // number of bits in dummy leads before pixel read
      cfg.dummy_read_bits = 1;       // number of bits in dummy leads before reading non-pixel data
      cfg.readable = false;          // set to true if data can be read
      cfg.invert = true;            // set to true if the light and dark of the panel is reversed
      cfg.rgb_order = false;          // set to true if the red and blue of the panel are swapped
      cfg.dlen_16bit = false;        // Set to true for panels that transmit data lengths in 16-bit increments in 16-bit parallel or SPI
      cfg.bus_shared = true;         // Set to true when sharing the bus with sd card (bus control is performed with drawJpgFile, etc.)

      // The following should only be set if the display is misalized by a driver with a variable number of pixels, such as st7735 or ILI9163.
      // cfg.memory_width = 240; //Maximum width supported by driver ICs
      // cfg.memory_height = 320; //Maximum height supported by driver ICs

      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();

      cfg.pin_bl = TFT_BLK;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance); // Set the panel to be used.
  }
};

// declare display variable
static LGFX tft;
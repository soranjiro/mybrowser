# Test Pages

This directory contains various HTML test pages for testing browser functionality.

## Video and Picture-in-Picture Tests

### `video_test.html`

Basic video functionality test page with various video formats and controls.

### `pip_test.html`

Detailed Picture-in-Picture functionality test with:

- PiP API availability detection
- Manual PiP trigger buttons
- Video element analysis
- Debug console output

### `pip_integration_test.html`

Comprehensive PiP integration test with:

- Multiple video sources
- Automatic attribute removal testing
- Browser compatibility checks
- Performance monitoring

## General Tests

### `test_page.html`

General browser functionality test page.

### `debug_test.html`

Debug and development test page for various browser features.

## Usage

1. Build and run MyBrowser
2. Navigate to these test pages using file:// URLs
3. Test the specific functionality described in each page
4. Check browser console for debug output

## Notes

- Some tests require specific Chromium flags (automatically set in main.cpp)
- PiP functionality may have limitations in Qt WebEngine
- Test results may vary depending on the Qt and Chromium versions

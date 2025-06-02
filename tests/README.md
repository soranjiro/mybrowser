# Test Pages

This directory contains minimal test pages for testing browser functionality.

## Test Pages

### `click_test.html`

Comprehensive click functionality test with:

- Button click testing
- Link click testing (normal, JavaScript, internal, external)
- Nested element click event testing with stopPropagation
- Mouse event debugging and logging
- Visual feedback for interactions

### `pip_test_comprehensive.html`

Complete Picture-in-Picture functionality test with:

- Normal video with PiP enabled
- Video with `disablePictureInPicture="true"` attribute
- Local video file loading support
- PiP API availability detection
- Manual PiP control buttons
- Video state analysis and debugging
- Comprehensive event logging

## Usage

1. Build and run MyBrowser
2. Navigate to these test pages using file:// URLs
3. Test the specific functionality described in each page
4. Check browser console for debug output

## Notes

- Some tests require specific Chromium flags (automatically set in main.cpp)
- PiP functionality may have limitations in Qt WebEngine
- Test results may vary depending on the Qt and Chromium versions

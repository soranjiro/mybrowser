// WebView Enhancement JavaScript

class WebViewEnhancer {
    constructor(debugMode = false) {
        this.debugMode = debugMode;
        this.init();
    }

    init() {
        if (this.debugMode) {
            console.log('WebView: Initializing enhancement scripts');
        }

        this.setupJavaScriptLinkHandling();
        this.setupClickDebugging();
        this.setupMutationObserver();
    }

    // JavaScriptリンクの高優先度ハンドリング
    setupJavaScriptLinkHandling() {
        // 最高優先度でJavaScriptリンクを処理
        document.addEventListener('click', (e) => {
            if (e.target.tagName === 'A' && e.target.href && e.target.href.startsWith('javascript:')) {
                if (this.debugMode) {
                    console.log('WebView High Priority Handler - Click detected:', {
                        tagName: e.target.tagName,
                        href: e.target.href,
                        fullHref: e.target.href ? e.target.href.toString() : 'no href',
                        textContent: e.target.textContent,
                        target: e.target.target,
                        x: e.clientX,
                        y: e.clientY,
                        defaultPrevented: e.defaultPrevented,
                        timeStamp: e.timeStamp
                    });
                    console.log('WebView High Priority: JavaScript link detected, executing immediately...');
                    console.log('WebView High Priority: Full href:', e.target.href);
                }

                try {
                    var jsCode = decodeURIComponent(e.target.href.substring(11));
                    if (this.debugMode) {
                        console.log('WebView High Priority: Executing JavaScript code:', jsCode);
                    }

                    // Execute the JavaScript code in global scope
                    var result = eval.call(window, jsCode);
                    if (this.debugMode) {
                        console.log('WebView High Priority: JavaScript executed successfully, result:', result);
                    }
                } catch (error) {
                    console.error('WebView High Priority: Error executing JavaScript link:', error);
                    console.error('WebView High Priority: JavaScript code was:', jsCode);
                }

                // Prevent default and stop propagation for JavaScript links
                e.preventDefault();
                e.stopPropagation();
                return false;
            }
        }, true); // Use capture phase for highest priority

        // 通常の優先度でリンクハンドリング
        document.addEventListener('click', (e) => {
            if (e.target.tagName === 'A' && e.target.href) {
                if (this.debugMode) {
                    console.log('WebView Enhanced Link Handler - Click detected:', {
                        href: e.target.href,
                        target: e.target.target,
                        hash: e.target.hash,
                        hostname: e.target.hostname,
                        pathname: e.target.pathname,
                        defaultPrevented: e.defaultPrevented
                    });
                }

                // Handle JavaScript links explicitly
                if (e.target.href.startsWith('javascript:')) {
                    if (this.debugMode) {
                        console.log('WebView: JavaScript link detected, executing...');
                        console.log('WebView: Full href:', e.target.href);
                    }
                    try {
                        // Extract and execute the JavaScript code
                        var jsCode = decodeURIComponent(e.target.href.substring(11)); // Remove 'javascript:'
                        if (this.debugMode) {
                            console.log('WebView: Executing JavaScript code:', jsCode);
                        }

                        // Multiple approaches to ensure execution works
                        // Method 1: Direct eval (global scope)
                        try {
                            eval.call(window, jsCode);
                            if (this.debugMode) {
                                console.log('WebView: JavaScript executed successfully with eval.call');
                            }
                        } catch (evalError) {
                            if (this.debugMode) {
                                console.log('WebView: eval.call failed, trying Function constructor');
                            }
                            // Method 2: Function constructor (safer and more reliable)
                            var func = new Function(jsCode);
                            func.call(window);
                            if (this.debugMode) {
                                console.log('WebView: JavaScript executed successfully with Function constructor');
                            }
                        }
                    } catch (error) {
                        console.error('WebView: Error executing JavaScript link:', error);
                        console.error('WebView: JavaScript code was:', jsCode);
                    }
                    // Prevent default navigation for JavaScript links only
                    e.preventDefault();
                    e.stopPropagation();
                    return;
                }

                // For fragment links (same page navigation)
                if (e.target.hash && (e.target.hostname === window.location.hostname || e.target.hostname === '')) {
                    if (this.debugMode) {
                        console.log('WebView: Fragment link detected, allowing normal navigation');
                    }
                    // Let the browser handle fragment navigation naturally
                    return;
                }

                // For external links
                if (e.target.target === '_blank' || (e.target.hostname && e.target.hostname !== window.location.hostname)) {
                    if (this.debugMode) {
                        console.log('WebView: External link detected, allowing normal navigation');
                    }
                    // Let the browser handle external links naturally
                    return;
                }

                // For regular links (same hostname)
                if (this.debugMode) {
                    console.log('WebView: Regular link detected, allowing normal navigation');
                }
                // Let the browser handle all other links naturally
                return;
            }

            // For non-link elements, don't interfere with their click handling
            if (this.debugMode && e.target.tagName !== 'A') {
                console.log('WebView: Non-link click detected:', {
                    tagName: e.target.tagName,
                    type: e.target.type,
                    className: e.target.className,
                    defaultPrevented: e.defaultPrevented
                });
            }
        }, false); // Use bubbling phase, lower priority
    }

    // クリックイベントのデバッグ機能
    setupClickDebugging() {
        if (!this.debugMode) return;

        // Debug all click events (only in debug mode)
        document.addEventListener('click', (e) => {
            console.log('JS Click Event:', {
                target: e.target.tagName,
                className: e.target.className,
                href: e.target.href,
                fullUrl: e.target.href ? e.target.href.toString() : 'no href',
                textContent: e.target.textContent,
                x: e.clientX,
                y: e.clientY,
                button: e.button,
                ctrlKey: e.ctrlKey,
                metaKey: e.metaKey,
                shiftKey: e.shiftKey,
                altKey: e.altKey,
                defaultPrevented: e.defaultPrevented,
                isTrusted: e.isTrusted
            });
        }, true);

        // Debug mousedown events
        document.addEventListener('mousedown', (e) => {
            console.log('JS MouseDown Event:', {
                target: e.target.tagName,
                className: e.target.className,
                x: e.clientX,
                y: e.clientY,
                button: e.button,
                defaultPrevented: e.defaultPrevented
            });
        }, true);

        // Debug mouseup events
        document.addEventListener('mouseup', (e) => {
            console.log('JS MouseUp Event:', {
                target: e.target.tagName,
                className: e.target.className,
                x: e.clientX,
                y: e.clientY,
                button: e.button,
                defaultPrevented: e.defaultPrevented
            });
        }, true);

        // Additional debug for focus events
        document.addEventListener('focus', (e) => {
            console.log('JS Focus Event:', e.target.tagName);
        }, true);

        document.addEventListener('blur', (e) => {
            console.log('JS Blur Event:', e.target.tagName);
        }, true);
    }

    // MutationObserverで動的コンテンツを監視
    setupMutationObserver() {
        // MutationObserver to handle dynamically added content
        const observer = new MutationObserver((mutations) => {
            mutations.forEach((mutation) => {
                if (mutation.type === 'childList') {
                    mutation.addedNodes.forEach((node) => {
                        if (node.nodeType === 1) { // Element node
                            // Handle videos with disablepictureinpicture
                            const videos = node.tagName === 'VIDEO' ? [node] :
                                          node.querySelectorAll ? node.querySelectorAll('video') : [];

                            videos.forEach((video) => {
                                if (video.hasAttribute('disablepictureinpicture')) {
                                    if (this.debugMode) {
                                        console.log('WebView: Automatically removing disablepictureinpicture from:', video);
                                    }
                                    video.removeAttribute('disablepictureinpicture');
                                }
                            });
                        }
                    });
                }
            });
        });

        // Start observing
        observer.observe(document.body, {
            childList: true,
            subtree: true
        });
    }

    // PiP属性の自動削除
    removeDisablePiPAttributes() {
        const videos = document.querySelectorAll('video[disablepictureinpicture]');
        videos.forEach(video => {
            if (this.debugMode) {
                console.log('WebView: Removing disablepictureinpicture from:', video);
            }
            video.removeAttribute('disablepictureinpicture');
        });
    }
}

// 初期化（デバッグモードは設定により変更可能）
window.webViewEnhancer = new WebViewEnhancer(false);

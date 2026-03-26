/**
 * Phoenix Engine React Component
 * 
 * TypeScript React wrapper for Phoenix Engine WebAssembly
 */

import React, { 
    useRef, 
    useEffect, 
    useState, 
    useCallback,
    forwardRef,
    useImperativeHandle
} from 'react';

// Types
interface PhoenixEngineConfig {
    width?: number;
    height?: number;
    enableWebGPU?: boolean;
    enableWebGL2?: boolean;
    enableAsyncify?: boolean;
    wasmPath?: string;
}

interface FrameInfo {
    deltaTime: number;
    frameTime: number;
    frameNumber: bigint;
    fps: number;
}

interface GraphicsCaps {
    hasWebGPU: boolean;
    hasWebGL2: boolean;
    hasWebGL: boolean;
    maxTextureSize: number;
}

interface PhoenixEngineProps {
    config?: PhoenixEngineConfig;
    className?: string;
    style?: React.CSSProperties;
    onInit?: (engine: any) => void;
    onFrame?: (frameInfo: FrameInfo) => void;
    onError?: (error: Error) => void;
    onLoad?: () => void;
    autoStart?: boolean;
    children?: React.ReactNode;
}

export interface PhoenixEngineRef {
    start: () => void;
    stop: () => void;
    resize: (width: number, height: number) => void;
    loadResource: (url: string, type: string) => Promise<any>;
    createScene: () => any;
    getGraphicsCaps: () => GraphicsCaps | null;
    isRunning: boolean;
    isInitialized: boolean;
}

/**
 * Phoenix Engine React Component
 */
export const PhoenixEngine = forwardRef<PhoenixEngineRef, PhoenixEngineProps>(
    (props, ref) => {
        const {
            config = {},
            className = '',
            style = {},
            onInit,
            onFrame,
            onError,
            onLoad,
            autoStart = true,
            children
        } = props;

        const canvasRef = useRef<HTMLCanvasElement>(null);
        const engineRef = useRef<any>(null);
        const animationFrameRef = useRef<number>(0);
        const [isInitialized, setIsInitialized] = useState(false);
        const [isRunning, setIsRunning] = useState(false);
        const [fps, setFps] = useState(0);
        const [error, setError] = useState<Error | null>(null);
        const [backend, setBackend] = useState<string>('');

        // Expose methods to parent via ref
        useImperativeHandle(ref, () => ({
            start: handleStart,
            stop: handleStop,
            resize: handleResize,
            loadResource: async (url: string, type: string) => {
                if (engineRef.current) {
                    return await engineRef.current.loadResource(url, type);
                }
                throw new Error('Engine not initialized');
            },
            createScene: () => {
                if (engineRef.current) {
                    return engineRef.current.createScene();
                }
                throw new Error('Engine not initialized');
            },
            getGraphicsCaps: () => {
                if (engineRef.current) {
                    return engineRef.current.getGraphicsCaps();
                }
                return null;
            },
            isRunning,
            isInitialized
        }));

        // Initialize engine
        useEffect(() => {
            let mounted = true;

            const initEngine = async () => {
                try {
                    // Load Phoenix Engine module
                    const { PhoenixEngine: EngineClass } = await import(
                        config.wasmPath || '../../js/phoenix-wasm-api'
                    );

                    const engine = new EngineClass();
                    engineRef.current = engine;

                    // Set callbacks
                    engine.setCallbacks({
                        onProgress: (progress: number) => {
                            console.log(`Loading: ${Math.round(progress * 100)}%`);
                        },
                        onLoad: () => {
                            if (!mounted) return;
                            setIsInitialized(true);
                            onLoad?.();
                            onInit?.(engine);

                            // Detect backend
                            const caps = engine.getGraphicsCaps();
                            if (caps) {
                                setBackend(
                                    caps.hasWebGPU ? 'WebGPU' :
                                    caps.hasWebGL2 ? 'WebGL2' : 'WebGL'
                                );
                            }

                            if (autoStart) {
                                handleStart();
                            }
                        },
                        onError: (err: Error) => {
                            if (!mounted) return;
                            setError(err);
                            onError?.(err);
                        },
                        onFrame: (frameInfo: FrameInfo) => {
                            if (!mounted) return;
                            setFps(frameInfo.fps);
                            onFrame?.(frameInfo);
                        }
                    });

                    // Initialize with config
                    await engine.init({
                        ...config,
                        canvasId: canvasRef.current || 'canvas'
                    });

                } catch (err) {
                    if (!mounted) return;
                    const error = err instanceof Error ? err : new Error(String(err));
                    setError(error);
                    onError?.(error);
                }
            };

            initEngine();

            return () => {
                mounted = false;
                if (engineRef.current) {
                    engineRef.current.shutdown();
                }
                if (animationFrameRef.current) {
                    cancelAnimationFrame(animationFrameRef.current);
                }
            };
        }, []);

        // Handle resize
        useEffect(() => {
            const handleWindowResize = () => {
                if (canvasRef.current && engineRef.current) {
                    handleResize(
                        canvasRef.current.clientWidth,
                        canvasRef.current.clientHeight
                    );
                }
            };

            window.addEventListener('resize', handleWindowResize);
            return () => window.removeEventListener('resize', handleWindowResize);
        }, [isInitialized]);

        const handleStart = useCallback(() => {
            if (engineRef.current && !isRunning) {
                engineRef.current.start();
                setIsRunning(true);
            }
        }, [isRunning]);

        const handleStop = useCallback(() => {
            if (engineRef.current && isRunning) {
                engineRef.current.stop();
                setIsRunning(false);
            }
        }, [isRunning]);

        const handleResize = useCallback((width: number, height: number) => {
            if (engineRef.current) {
                engineRef.current.resize(width, height);
            }
        }, []);

        return (
            <div 
                className={`phoenix-engine-container ${className}`}
                style={{
                    position: 'relative',
                    width: '100%',
                    height: '100%',
                    ...style
                }}
            >
                <canvas
                    ref={canvasRef}
                    id="phoenix-canvas"
                    style={{
                        display: 'block',
                        width: '100%',
                        height: '100%'
                    }}
                />
                
                {/* Overlay UI */}
                {isInitialized && (
                    <div style={styles.overlay}>
                        <div style={styles.stats}>
                            <span style={styles.stat}>FPS: {fps.toFixed(1)}</span>
                            <span style={styles.stat}>Backend: {backend}</span>
                        </div>
                    </div>
                )}

                {/* Loading State */}
                {!isInitialized && !error && (
                    <div style={styles.loading}>
                        <div style={styles.spinner} />
                        <p>Loading Phoenix Engine...</p>
                    </div>
                )}

                {/* Error State */}
                {error && (
                    <div style={styles.error}>
                        <h3>Engine Error</h3>
                        <p>{error.message}</p>
                        <button onClick={() => window.location.reload()}>
                            Retry
                        </button>
                    </div>
                )}

                {/* Children (custom UI) */}
                {children}
            </div>
        );
    }
);

PhoenixEngine.displayName = 'PhoenixEngine';

// Styles
const styles: { [key: string]: React.CSSProperties } = {
    overlay: {
        position: 'absolute',
        top: 20,
        left: 20,
        background: 'rgba(0, 0, 0, 0.7)',
        padding: '15px 20px',
        borderRadius: '8px',
        backdropFilter: 'blur(10px)',
        color: '#fff',
        fontFamily: 'system-ui, -apple-system, sans-serif'
    },
    stats: {
        display: 'flex',
        gap: '20px',
        fontSize: '14px'
    },
    stat: {
        fontWeight: 600
    },
    loading: {
        position: 'absolute',
        top: '50%',
        left: '50%',
        transform: 'translate(-50%, -50%)',
        textAlign: 'center',
        color: '#fff',
        fontFamily: 'system-ui, -apple-system, sans-serif'
    },
    spinner: {
        width: '50px',
        height: '50px',
        border: '4px solid rgba(255, 255, 255, 0.1)',
        borderTopColor: '#00d4ff',
        borderRadius: '50%',
        animation: 'spin 1s linear infinite',
        margin: '0 auto 20px'
    },
    error: {
        position: 'absolute',
        top: '50%',
        left: '50%',
        transform: 'translate(-50%, -50%)',
        background: 'rgba(255, 68, 68, 0.9)',
        padding: '30px',
        borderRadius: '12px',
        textAlign: 'center',
        color: '#fff',
        fontFamily: 'system-ui, -apple-system, sans-serif',
        maxWidth: '400px'
    }
};

// Hook for using Phoenix Engine
export function usePhoenixEngine(config?: PhoenixEngineConfig) {
    const engineRef = useRef<any>(null);
    const [isInitialized, setIsInitialized] = useState(false);
    const [isRunning, setIsRunning] = useState(false);
    const [error, setError] = useState<Error | null>(null);

    const init = useCallback(async () => {
        try {
            const { PhoenixEngine: EngineClass } = await import(
                config?.wasmPath || '../../js/phoenix-wasm-api'
            );

            const engine = new EngineClass();
            await engine.init(config);
            
            engineRef.current = engine;
            setIsInitialized(true);
            
            return engine;
        } catch (err) {
            const error = err instanceof Error ? err : new Error(String(err));
            setError(error);
            throw error;
        }
    }, [config]);

    const start = useCallback(() => {
        if (engineRef.current) {
            engineRef.current.start();
            setIsRunning(true);
        }
    }, []);

    const stop = useCallback(() => {
        if (engineRef.current) {
            engineRef.current.stop();
            setIsRunning(false);
        }
    }, []);

    const shutdown = useCallback(() => {
        if (engineRef.current) {
            engineRef.current.shutdown();
            engineRef.current = null;
            setIsInitialized(false);
            setIsRunning(false);
        }
    }, []);

    return {
        engine: engineRef.current,
        isInitialized,
        isRunning,
        error,
        init,
        start,
        stop,
        shutdown
    };
}

export default PhoenixEngine;

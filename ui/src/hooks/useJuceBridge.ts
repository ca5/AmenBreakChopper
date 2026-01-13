import { useEffect, useState, useCallback, useRef } from 'react';

// Define the shape of the global JUCE object/functions
declare global {
    interface Window {
        // Functions sent FROM JUCE to JS
        juce_updateParameter?: (data: { id: string; value: number }) => void;
        juce_emitEvent?: (type: string, data: any) => void;

        // Functions sent FROM JS to JUCE (Native Functions)
        sendParameterValue?: (id: string, value: number) => void;
        performSequenceReset?: () => void;
        performSoftReset?: () => void;
        performHardReset?: () => void;
        triggerNoteFromUi?: (noteNumber: number) => void;
        requestInitialState?: () => void;
        getDeviceList?: () => void;
        setAudioDevice?: (name: string) => void;
        setMidiInput?: (id: string, enabled: boolean) => void;
        __JUCE__?: {
            postMessage: (data: string) => void;
        };
    }
}

// Helper to invoke native functions via low-level IPC
const invokeNative = (name: string, ...args: any[]) => {
    if (window.__JUCE__ && window.__JUCE__.postMessage) {
        const payload = {
            name: name,
            params: args,
            resultId: Date.now()
        };
        window.__JUCE__.postMessage(JSON.stringify({
            eventId: "__juce__invoke",
            payload: payload
        }));
    } else {
        console.warn(`[Bridge] Cannot invoke ${name}: Not in JUCE environment.`);
    }
};

type EventCallback = (data: any) => void;
type ParameterCallback = (id: string, value: number) => void;

// --- Global Singleton State (Module Scope) ---
const globalListeners: Record<string, Set<EventCallback>> = {};
const globalParamListeners: Set<ParameterCallback> = new Set();
const globalEnvListeners: Set<(isStandalone: boolean) => void> = new Set();
let globalParameters: Record<string, number> = {};
let isBridgeInitialized = false;
let envIsStandalone = false; // Default to false (Plugin mode preferred default)

const initBridge = () => {
    if (isBridgeInitialized) return;
    isBridgeInitialized = true;

    // 1. Initial Polyfill / Setup
    if (typeof window.sendParameterValue === 'undefined') {
        if (window.__JUCE__ && window.__JUCE__.postMessage) {
            window.sendParameterValue = (id: string, value: number) => {
                const payload = {
                    name: "sendParameterValue",
                    params: [id, value],
                    resultId: 0
                };
                window.__JUCE__!.postMessage(JSON.stringify({
                    eventId: "__juce__invoke",
                    payload: payload
                }));
            };
            console.log("[Bridge] Polyfill for sendParameterValue applied.");
        }
    }

    // 2. Bind Window Callbacks (Once)
    window.juce_updateParameter = (data: { id: string; value: number }) => {
        // Update global cache
        globalParameters[data.id] = data.value;
        // Notify all hooks
        globalParamListeners.forEach(cb => cb(data.id, data.value));
    };

    window.juce_emitEvent = (type: string, data: any) => {
        // Intercept Environment/State events
        if (type === 'environment') {
            if (data && typeof data.isStandalone === 'boolean') {
                envIsStandalone = data.isStandalone;
                globalEnvListeners.forEach(cb => cb(envIsStandalone));
                console.log(`[Bridge] Environment detected: ${envIsStandalone ? 'Standalone' : 'Plugin'}`);
            }
        }

        // Notify specific listeners
        if (globalListeners[type]) {
            globalListeners[type].forEach(cb => cb(data));
        }
    };
    
    // 3. Request Initial State
    if (window.requestInitialState) {
        window.requestInitialState();
    } else if (window.__JUCE__ && window.__JUCE__.postMessage) {
         const payload = {
                name: "requestInitialState",
                params: [],
                resultId: 0
            };
            window.__JUCE__.postMessage(JSON.stringify({
                eventId: "__juce__invoke",
                payload: payload
            }));
    }
};

export const useJuceBridge = () => {
    // Local state for this component
    const [parameters, setParameters] = useState<Record<string, number>>(globalParameters);
    const [isStandalone, setIsStandalone] = useState(envIsStandalone);

    useEffect(() => {
        // Ensure bridge is set up
        initBridge();

        // Subscribe to parameter updates
        const handleParamUpdate: ParameterCallback = (id, value) => {
            setParameters(prev => ({ ...prev, [id]: value }));
        };
        globalParamListeners.add(handleParamUpdate);
        
        // Subscribe to environment updates
        const handleEnvUpdate = (val: boolean) => setIsStandalone(val);
        globalEnvListeners.add(handleEnvUpdate);

        // Sync local state
        setParameters({ ...globalParameters });
        setIsStandalone(envIsStandalone);

        return () => {
            globalParamListeners.delete(handleParamUpdate);
            globalEnvListeners.delete(handleEnvUpdate);
        };
    }, []);

    const addEventListener = useCallback((type: string, callback: EventCallback) => {
        if (!globalListeners[type]) {
            globalListeners[type] = new Set();
        }
        globalListeners[type].add(callback);

        return () => {
            if (globalListeners[type]) {
                globalListeners[type].delete(callback);
            }
        };
    }, []);

    // Function to send updates to JUCE
    const sendParameter = useCallback((id: string, value: number) => {
        // Optimistic update global
        globalParameters[id] = value;
        // Optimistic update local
        setParameters((prev) => ({
            ...prev,
            [id]: value,
        }));
        // Notify other components? loopback? 
        // For now, let's assume they update via re-render or wait for echo.
        // Actually best to notify others immediately too:
        globalParamListeners.forEach(cb => cb(id, value));

        if (window.sendParameterValue) {
            window.sendParameterValue(id, value);
        }
    }, []);

    // ... Helper wrappers ...
    const performSequenceReset = useCallback(() => {
        if (window.performSequenceReset) window.performSequenceReset();
        else if (window.__JUCE__?.postMessage) {
             window.__JUCE__.postMessage(JSON.stringify({ eventId: "__juce__invoke", payload: { name: "performSequenceReset", params: [], resultId: 0 } }));
        }
    }, []);

    const performSoftReset = useCallback(() => {
        if (window.performSoftReset) window.performSoftReset();
        else if (window.__JUCE__?.postMessage) {
             window.__JUCE__.postMessage(JSON.stringify({ eventId: "__juce__invoke", payload: { name: "performSoftReset", params: [], resultId: 0 } }));
        }
    }, []);
    
    const performHardReset = useCallback(() => {
        if (window.performHardReset) window.performHardReset();
        else if (window.__JUCE__?.postMessage) {
             window.__JUCE__.postMessage(JSON.stringify({ eventId: "__juce__invoke", payload: { name: "performHardReset", params: [], resultId: 0 } }));
        }
    }, []);

    const triggerNote = useCallback((noteNumber: number) => {
        if (window.triggerNoteFromUi) window.triggerNoteFromUi(noteNumber);
        else if (window.__JUCE__?.postMessage) {
             window.__JUCE__.postMessage(JSON.stringify({ eventId: "__juce__invoke", payload: { name: "triggerNoteFromUi", params: [noteNumber], resultId: 0 } }));
        }
    }, []);

    const getDeviceList = useCallback(() => {
        if (window.getDeviceList) window.getDeviceList();
        else if (window.__JUCE__?.postMessage) {
             window.__JUCE__.postMessage(JSON.stringify({ eventId: "__juce__invoke", payload: { name: "getDeviceList", params: [], resultId: 0 } }));
        }
    }, []);

    const setAudioDevice = useCallback((name: string) => {
        if (window.setAudioDevice) window.setAudioDevice(name);
        else if (window.__JUCE__?.postMessage) {
             window.__JUCE__.postMessage(JSON.stringify({ eventId: "__juce__invoke", payload: { name: "setAudioDevice", params: [name], resultId: 0 } }));
        }
    }, []);

    const setMidiInput = useCallback((id: string, enabled: boolean) => {
        if (window.setMidiInput) window.setMidiInput(id, enabled);
        else if (window.__JUCE__?.postMessage) {
             window.__JUCE__.postMessage(JSON.stringify({ eventId: "__juce__invoke", payload: { name: "setMidiInput", params: [id, enabled], resultId: 0 } }));
        }
    }, []);

    const openBluetoothPairingDialog = useCallback(() => {
        console.log("Frontend: openBluetoothPairingDialog called via invokeNative");
        
        if (window.__JUCE__ && window.__JUCE__.postMessage) {
            const payload = {
                name: "openBluetoothPairingDialog",
                params: [],
                resultId: Date.now()
            };
            window.__JUCE__.postMessage(JSON.stringify({
                eventId: "__juce__invoke",
                payload: payload
            }));
        } else {
            console.warn("[Bridge] Cannot invoke openBluetoothPairingDialog: Not in JUCE environment.");
        }
    }, []);

    return {
        parameters,
        isStandalone,
        sendParameter,
        sendParameterValue: sendParameter, // Alias
        addEventListener,
        performSequenceReset,
        performSoftReset,
        performHardReset,
        triggerNote,
        getDeviceList,
        setAudioDevice,
        setMidiInput,
        openBluetoothPairingDialog
    };
};

// Set a specific input channel (by index) for the current device
export const setAudioInputChannel = (channelIndex: number): Promise<void> => {
    invokeNative("setAudioInputChannel", channelIndex);
    return Promise.resolve();
};

export const openBluetoothPairingDialog = () => {
    console.log("Frontend: openBluetoothPairingDialog called via invokeNative");
    
    if (window.__JUCE__ && window.__JUCE__.postMessage) {
        const payload = {
            name: "openBluetoothPairingDialog",
            params: [],
            resultId: Date.now()
        };
        window.__JUCE__.postMessage(JSON.stringify({
            eventId: "__juce__invoke",
            payload: payload
        }));
    } else {
        console.warn("[Bridge] Cannot invoke openBluetoothPairingDialog: Not in JUCE environment.");
    }
};

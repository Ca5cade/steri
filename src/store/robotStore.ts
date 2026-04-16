import { create } from 'zustand';
import { ReemanAPI } from '../api/reeman';

interface RobotState {
    host: string;
    hostname: string;
    isConnected: boolean;
    api: ReemanAPI | null;
    batteryLevel: number;
    currentSpeed: number;
    maxSpeed: number;
    currentMode: number;
    robotPose: { x: number; y: number; theta: number } | null;
    laserData: any[];

    setHost: (host: string) => void;
    connect: () => Promise<boolean>;
    disconnect: () => void;
    updateStatus: () => Promise<void>;
    setNavigationMaximumSpeed: (speed: number) => Promise<boolean>;
    startDocking: () => Promise<boolean>;
}

export const useRobotStore = create<RobotState>((set, get) => ({
    host: '192.168.1.100', // Default IP
    hostname: '',
    isConnected: false,
    api: null,
    batteryLevel: 0,
    currentSpeed: 0,
    maxSpeed: 0.6,
    currentMode: 0,
    robotPose: null,
    laserData: [],

    setHost: (host) => set({ host }),

    connect: async () => {
        const { host } = get();
        const api = new ReemanAPI(host);
        const { data, error } = await api.getCurrentVersion();

        if (!error && data) {
            set({ isConnected: true, api });
            // Fetch hostname immediately after connection
            const { data: hostnameData } = await api.getMachineHostname();
            if (hostnameData && hostnameData.hostname) {
                set({ hostname: hostnameData.hostname });
            }
            return true;
        } else {
            set({ isConnected: false, api: null });
            return false;
        }
    },

    disconnect: () => set({ isConnected: false, api: null, hostname: '' }),

    updateStatus: async () => {
        const { api, isConnected } = get();
        if (!isConnected || !api) return;

        // Fetch pose
        const { data: poseData } = await api.getRobotPose();
        if (poseData) {
            set({ robotPose: poseData });
        }

        // Fetch mode
        const { data: modeData } = await api.getCurrentMode();
        if (modeData) {
            set({ currentMode: modeData.mode });
        }

        // Fetch battery (power management)
        const { data: powerData } = await api.getPowerManagementStatus();
        if (powerData && typeof powerData.battery !== 'undefined') {
            set({ batteryLevel: powerData.battery });
        }

        // Fetch current speed
        const { data: speedData } = await api.getCurrentSpeed();
        if (speedData && typeof speedData.vx !== 'undefined') {
            // vx is linear velocity in m/s
            set({ currentSpeed: Math.abs(speedData.vx) });
        }

        // Fetch laser data
        const { data: laserData } = await api.getLaserData();
        if (laserData && laserData.coordinates) {
            set({ laserData: laserData.coordinates });
        }

        // Fetch max speed
        const { data: maxSpeedData } = await api.getNavigationMaximumSpeed();
        if (maxSpeedData && typeof maxSpeedData.speed !== 'undefined') {
            set({ maxSpeed: maxSpeedData.speed });
        }
    },

    setNavigationMaximumSpeed: async (speed: number) => {
        const { api, isConnected } = get();
        if (!isConnected || !api) return false;

        const { error } = await api.setNavigationMaximumSpeed(speed);
        return !error;
    },

    startDocking: async () => {
        const { api, isConnected } = get();
        if (!isConnected || !api) return false;

        const { error } = await api.dockingCharging();
        return !error;
    },
}));

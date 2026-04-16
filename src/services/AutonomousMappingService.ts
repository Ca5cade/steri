import { ReemanAPI } from '../api/reeman';

export class AutonomousMappingService {
    private api: ReemanAPI;
    private isRunning: boolean = false;
    private resolution: number = 0.05;
    private thresholdFront: number = 0.35;
    private thresholdSide: number = 0.30;
    private scanAngleWidth: number = 30; // degrees

    constructor(api: ReemanAPI) {
        this.api = api;
    }

    async start() {
        if (this.isRunning) return;
        this.isRunning = true;

        // Force reset to mode 0 to clear previous state
        await this.api.switchMappingMode(0);
        await this.sleep(1000);

        // Switch to mapping mode (mode 1)
        const { error } = await this.api.switchMappingMode(1);
        if (error) {
            console.error('Failed to switch to mapping mode:', error);
            this.isRunning = false;
            return;
        }

        this.loop();
    }

    stop() {
        this.isRunning = false;
        this.api.setSpeed(0, 0);
    }

    private async loop() {
        let stepStartTime = Date.now();
        const STEP_DURATION = 3000; // 3 seconds walking
        const TURN_SPEED = 0.6;
        const WALK_SPEED = 0.3;

        while (this.isRunning) {
            try {
                // 1. Walking State Logic (implicit default check)
                const isWalkingTime = Date.now() - stepStartTime < STEP_DURATION;

                // Get data for Safety Check
                const { data: laserData, error } = await this.api.getLaserData();
                if (error || !laserData || !laserData.coordinates) {
                    await this.api.setSpeed(0, 0);
                    await this.sleep(200);
                    continue;
                }

                const { frontMin } = this.analyzeExits(laserData.coordinates);

                // EMERGENCY SAFETY CHECK
                if (frontMin < this.thresholdFront) {
                    // Obstacle too close! Stop and trigger scan immediately
                    console.log('Obstacle detected! Stopping and Scanning...');
                    await this.api.setSpeed(0, 0);
                    stepStartTime = 0; // Force timeout
                    // Fall through to scan logic naturally if we change state handling, 
                    // but here we just let the while loop handle the "not walking time" next iteration
                    // Actually, let's force the scan phase now.
                } else if (isWalkingTime) {
                    // CONTINUE WALKING
                    await this.api.setSpeed(WALK_SPEED, 0);
                    await this.sleep(100);
                    continue; // Skip the rest of the loop to keep walking
                }

                // --- SCANNING & DECIDING PHASE ---
                // If we are here, either timeout or obstacle stopped us.

                console.log('Scanning Phase Initiated...');
                await this.api.setSpeed(0, 0); // Stop first
                await this.sleep(500);

                // 2. Look Around (The "Look 180" behavior)
                // Turn Left ~90
                if (!this.isRunning) break;
                await this.performTurn(TURN_SPEED, 1.5); // ~1.5s at 0.6 rad/s ~= 0.9 rad ~= 50deg? adjust timing
                await this.sleep(200);

                // Turn Right ~180 (scan other side)
                if (!this.isRunning) break;
                await this.performTurn(-TURN_SPEED, 3.0);
                await this.sleep(200);

                // Return to Center ~90
                if (!this.isRunning) break;
                await this.performTurn(TURN_SPEED, 1.5);
                await this.sleep(500);

                // 3. Make Decision
                // Fetch fresh data after looking around
                const { data: freshData } = await this.api.getLaserData();
                if (!freshData || !freshData.coordinates) {
                    continue;
                }

                const bestDir = this.findBestDirection(freshData.coordinates);
                console.log(`Decision: Turn ${bestDir} degrees`);

                // 4. Aligning
                if (Math.abs(bestDir) > 5) {
                    // Simple open-loop turn: time = angle / speed
                    // angle in rad = bestDir * PI / 180
                    // time = angleRad / TURN_SPEED
                    const angleRad = Math.abs(bestDir) * (Math.PI / 180);
                    const turnDir = bestDir > 0 ? 1 : -1;
                    const turnTimeSec = angleRad / TURN_SPEED;

                    // Cap turn time to avoid spinning forever if calculation is off
                    const safeTime = Math.min(turnTimeSec, 3.0);

                    await this.performTurn(turnDir * TURN_SPEED, safeTime);
                }

                // Reset Step Timer
                stepStartTime = Date.now();
                await this.sleep(200);

            } catch (e) {
                console.error('Error in autonomous loop:', e);
                await this.sleep(1000);
            }
        }
        // Ensure stopped when exiting loop
        await this.api.setSpeed(0, 0);
    }

    private async performTurn(speed: number, durationSec: number) {
        if (!this.isRunning) return;
        const ms = durationSec * 1000;
        const steps = 5; // Send command a few times to ensure it sticks
        const interval = ms / steps;

        for (let i = 0; i < steps; i++) {
            if (!this.isRunning) break;
            await this.api.setSpeed(0, speed);
            await this.sleep(interval);
        }
        await this.api.setSpeed(0, 0);
    }

    private analyzeExits(coordinates: number[][]) {
        let frontMin = Infinity;

        for (const point of coordinates) {
            if (point.length !== 2) continue;
            const dist = Math.sqrt(point[0] * point[0] + point[1] * point[1]);
            const angle = Math.atan2(point[1], point[0]) * 180 / Math.PI;

            if (dist < 0.05) continue;

            // Front sector +/- 20 deg
            if (Math.abs(angle) < 20) {
                frontMin = Math.min(frontMin, dist);
            }
        }
        return { frontMin };
    }

    private findBestDirection(coordinates: number[][]): number {
        // Divide -90 to +90 into sectors
        const sectors = 18; // 10 degrees each
        const sectorDistances = new Array(sectors).fill(0);
        const sectorCounts = new Array(sectors).fill(0);

        for (const point of coordinates) {
            const dist = Math.sqrt(point[0] * point[0] + point[1] * point[1]);
            if (dist > 5.0 || dist < 0.05) continue; // Ignore far noise and self

            const angle = Math.atan2(point[1], point[0]) * 180 / Math.PI;
            if (angle < -90 || angle > 90) continue;

            // Map angle -90..90 to index 0..17
            const idx = Math.floor((angle + 90) / 10);
            if (idx >= 0 && idx < sectors) {
                // We want to find the OPEN areas, so we care about the CLOSEST obstacle in each sector
                // Wait, if 0 is initialized, we need min logic.
                // Let's modify: init with a large value
            }
        }

        // Re-init for max-min logic
        const sectorMinDist = new Array(sectors).fill(Infinity);

        for (const point of coordinates) {
            const dist = Math.sqrt(point[0] * point[0] + point[1] * point[1]);
            if (dist < 0.05) continue;
            const angle = Math.atan2(point[1], point[0]) * 180 / Math.PI;

            if (angle >= -90 && angle < 90) {
                const idx = Math.floor((angle + 90) / 10);
                if (idx >= 0 && idx < sectors) {
                    sectorMinDist[idx] = Math.min(sectorMinDist[idx], dist);
                }
            }
        }

        // Find sector with largest min-distance (safest direction)
        let maxDist = -1;
        let bestIdx = -1;

        // Prefer center if distances are similar? 
        // Let's just pick the max.
        for (let i = 0; i < sectors; i++) {
            // Treat Infinity as "Max Sensor Range" e.g. 5m
            let d = sectorMinDist[i];
            if (d === Infinity) d = 5.0;

            if (d > maxDist) {
                maxDist = d;
                bestIdx = i;
            }
        }

        if (bestIdx === -1) return 180; // Turn around if blocked?

        // Convert index back to angle
        // Index 0 = -90..-80 -> center -85
        // center = -85 + idx * 10
        const bestAngle = -85 + bestIdx * 10;
        return bestAngle;
    }

    private sleep(ms: number) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
}

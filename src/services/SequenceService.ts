import { ReemanAPI } from '../api/reeman';

export interface SequencePoint {
    name: string;
    x: number;
    y: number;
    duration: number; // seconds to wait
}

export class SequenceService {
    private api: ReemanAPI;
    private isRunning: boolean = false;
    private sequence: SequencePoint[] = [];

    constructor(api: ReemanAPI) {
        this.api = api;
    }

    setSequence(points: SequencePoint[]) {
        this.sequence = points;
    }

    async start() {
        if (this.isRunning) return;
        if (this.sequence.length === 0) {
            console.warn("Sequence is empty.");
            return;
        }
        this.isRunning = true;
        this.runSequence();
    }

    stop() {
        this.isRunning = false;
        this.api.cancelNavigation();
    }

    private async runSequence() {
        console.log("Starting navigation sequence...");

        // Ensure navigation mode
        await this.api.switchMappingMode(2);
        await this.sleep(1000);

        for (let i = 0; i < this.sequence.length; i++) {
            if (!this.isRunning) break;

            const point = this.sequence[i];
            console.log(`Navigating to point ${i + 1}/${this.sequence.length}: ${point.name}`);

            // Get current theta
            const { data: pose } = await this.api.getRobotPose();
            const theta = pose?.theta || 0.0;

            const { error: navError } = await this.api.sendLocationCoordinateNavigation(point.x, point.y, theta);

            if (navError) {
                console.error(`Navigation to ${point.name} failed: ${navError}`);
                continue; // Skip to next point? Or stop? Python code skips.
            }

            // Wait for arrival
            let arrived = false;
            while (this.isRunning && !arrived) {
                const { data: status, error: statusError } = await this.api.getNavigationStatus();
                if (!statusError && status) {
                    const state = status.res;
                    if (state === 3) {
                        arrived = true;
                        console.log(`Arrived at ${point.name}.`);
                    } else if (state === 4) {
                        console.warn(`Navigation to ${point.name} failed/cancelled.`);
                        break;
                    }
                }
                await this.sleep(1000);
            }

            if (!this.isRunning) break;

            if (arrived && point.duration > 0) {
                console.log(`Waiting at ${point.name} for ${point.duration} seconds...`);
                await this.sleep(point.duration * 1000);
            }
        }

        console.log("Sequence finished.");
        this.isRunning = false;
    }

    private sleep(ms: number) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
}

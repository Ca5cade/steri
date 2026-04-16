import { ReemanAPI } from '../api/reeman';

export interface Point {
    x: number;
    y: number;
}

export class PatrolService {
    private api: ReemanAPI;
    private isRunning: boolean = false;
    private areaPoints: Point[] = [];

    constructor(api: ReemanAPI) {
        this.api = api;
    }

    setArea(points: Point[]) {
        this.areaPoints = points;
    }

    async start() {
        if (this.isRunning) return;
        if (this.areaPoints.length < 3) {
            console.warn("Patrol area not defined or too few points.");
            return;
        }
        this.isRunning = true;
        this.loop();
    }

    stop() {
        this.isRunning = false;
        this.api.cancelNavigation();
    }

    private async loop() {
        console.log("Starting patrol loop...");
        while (this.isRunning) {
            try {
                // 1. Generate random point
                const target = this.getRandomPointInArea();
                if (!target) {
                    console.warn("Could not find random point in area.");
                    await this.sleep(1000);
                    continue;
                }

                console.log(`Navigating to random point: (${target.x.toFixed(2)}, ${target.y.toFixed(2)})`);

                // 2. Navigate to point
                // Get current theta to preserve orientation or just use 0
                const { data: pose } = await this.api.getRobotPose();
                const theta = pose?.theta || 0.0;

                const { error: navError } = await this.api.sendLocationCoordinateNavigation(target.x, target.y, theta);

                if (navError) {
                    console.error(`Navigation failed: ${navError}`);
                    await this.sleep(1000);
                    continue;
                }

                // 3. Wait for arrival
                let arrived = false;
                while (this.isRunning && !arrived) {
                    const { data: status, error: statusError } = await this.api.getNavigationStatus();
                    if (!statusError && status) {
                        const state = status.res; // 3 = success, 4 = failed
                        if (state === 3) {
                            arrived = true;
                            console.log("Arrived at random point.");
                        } else if (state === 4) {
                            console.warn("Navigation to random point failed/cancelled.");
                            break; // Try next point
                        }
                    }
                    await this.sleep(1000);
                }

                if (!this.isRunning) break;

                // 4. Wait at point
                console.log("Waiting at point...");
                await this.sleep(5000); // Wait 5 seconds

            } catch (e) {
                console.error("Error in patrol loop:", e);
                await this.sleep(1000);
            }
        }
        console.log("Patrol stopped.");
    }

    private getRandomPointInArea(): Point | null {
        if (this.areaPoints.length < 3) return null;

        // Simple bounding box approach for now
        const minX = Math.min(...this.areaPoints.map(p => p.x));
        const maxX = Math.max(...this.areaPoints.map(p => p.x));
        const minY = Math.min(...this.areaPoints.map(p => p.y));
        const maxY = Math.max(...this.areaPoints.map(p => p.y));

        for (let i = 0; i < 100; i++) {
            const x = minX + Math.random() * (maxX - minX);
            const y = minY + Math.random() * (maxY - minY);
            const point = { x, y };

            if (this.isPointInPolygon(point, this.areaPoints)) {
                return point;
            }
        }
        return null;
    }

    private isPointInPolygon(point: Point, vs: Point[]): boolean {
        // Ray-casting algorithm for point in polygon
        let x = point.x, y = point.y;
        let inside = false;
        for (let i = 0, j = vs.length - 1; i < vs.length; j = i++) {
            let xi = vs[i].x, yi = vs[i].y;
            let xj = vs[j].x, yj = vs[j].y;

            let intersect = ((yi > y) !== (yj > y))
                && (x < (xj - xi) * (y - yi) / (yj - yi) + xi);
            if (intersect) inside = !inside;
        }
        return inside;
    }

    private sleep(ms: number) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
}

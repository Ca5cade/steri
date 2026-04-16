import axios, { AxiosInstance } from 'axios';

export class ReemanAPI {
  private baseURL: string;
  private cmdURL: string;
  private downloadURL: string;
  private uploadURL: string;
  private host: string;
  private client: AxiosInstance;

  constructor(host: string) {
    this.host = host;
    this.baseURL = `http://${host}/reeman`;
    this.cmdURL = `http://${host}/cmd`;
    this.downloadURL = `http://${host}/download`;
    this.uploadURL = `http://${host}/upload`;

    this.client = axios.create({
      timeout: 10000,
    });
  }

  private async get(endpoint: string, urlType: 'base' | 'host' = 'base'): Promise<any> {
    try {
      const url = urlType === 'base' ? `${this.baseURL}${endpoint}` : `http://${this.host}${endpoint}`;
      const response = await this.client.get(url);
      return { data: response.data, error: null };
    } catch (error: any) {
      return { data: null, error: error.message || 'Unknown error' };
    }
  }

  private async post(endpoint: string, data: any, urlType: 'cmd' | 'host' | null = 'cmd'): Promise<any> {
    try {
      let url: string;
      if (urlType === 'cmd') {
        url = `${this.cmdURL}${endpoint}`;
      } else if (urlType === 'host') {
        url = `http://${this.host}${endpoint}`;
      } else {
        url = endpoint; // Direct URL
      }

      const response = await this.client.post(url, data);
      return { data: response.data, error: null };
    } catch (error: any) {
      return { data: null, error: error.message || 'Unknown error' };
    }
  }

  // --- Basic Information ---
  async getCurrentVersion() {
    return this.get('/current_version');
  }

  async getRobotPose() {
    return this.get('/pose');
  }

  async getCurrentMode() {
    return this.get('/get_mode');
  }

  async getMachineHostname() {
    return this.get('/hostname');
  }

  async getImuState() {
    return this.get('/imu');
  }

  async getLaserData() {
    return this.get('/laser');
  }

  async getPowerManagementStatus() {
    return this.get('/base_encode');
  }

  async getCurrentSpeed() {
    return this.get('/speed');
  }

  // --- Relocation ---
  async relocPose(x: number, y: number, theta: number) {
    return this.post('/reloc_pose', { x, y, theta });
  }

  async relocAbsolute(x: number, y: number, theta: number) {
    return this.post('/reloc_absolute', { x, y, theta });
  }

  // --- Virtual Walls ---
  async getRestrictLayer() {
    return this.get('/restrict_layer');
  }

  async updateRestrictLayer(waypoints: any[]) {
    return this.post('/restrict_layer', { waypoints });
  }

  // --- Navigation Module ---
  async sendLocationCoordinateNavigation(x: number, y: number, theta: number) {
    return this.post('/nav', { x, y, theta });
  }

  async sendTargetNameNavigation(pointName: string) {
    return this.post('/nav_name', { point: pointName });
  }

  async sendFixedRoutePointNameNavigation(name: string) {
    return this.post('/points_path', { name });
  }

  async getNavigationStatus() {
    return this.get('/nav_status');
  }

  async getGlobalPlan() {
    return this.get('/global_plan');
  }

  async cancelNavigation() {
    return this.post('/cancel_goal', {});
  }

  async getNavigationMaximumSpeed() {
    return this.get('/max_speed');
  }

  async setNavigationMaximumSpeed(speed: number) {
    return this.post('/max_speed', { speed });
  }

  async dockingCharging() {
    return this.post('/charge', { type: 0, point: 'Charging pile' });
  }

  // --- Map Module ---
  async getCurrentMap() {
    return this.get('/map');
  }

  async switchMappingMode(mode: number) {
    // mode 1: mapping, 3: incremental mapping, 2: navigation
    return this.post('/set_mode', { mode });
  }

  async saveMap() {
    return this.post('/save_map', {});
  }

  async getCurrentMapName() {
    return this.get('/current_map');
  }

  async getAllMaps() {
    return this.get('/history_map');
  }

  async applyMap(mapName: string) {
    return this.post('/apply_map', { name: mapName });
  }

  async renameMap(oldName: string, newAlias: string) {
    return this.post('/rename_map', { name: oldName, alias: newAlias });
  }

  async deleteMap(mapName: string) {
    return this.post('/delete_map', { name: mapName });
  }

  // --- Movement Control ---
  async setSpeed(vx: number, vth: number) {
    return this.post('/speed', { vx, vth });
  }

  async move(distance: number, direction: number, speed: number) {
    const url = `http://${this.host}/cmd/move`;
    return this.post(url, { distance, direction, speed }, null);
  }

  async turn(angle: number, direction: number, speed: number) {
    const url = `http://${this.host}/cmd/turn`;
    return this.post(url, { angle, direction, speed }, null);
  }
}

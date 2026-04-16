/**
 * API Service for SteriBot Mobile
 * Connects to the same backend as v1 web application
 */

const DEFAULT_BASE_URL = 'http://localhost:3001/api/v1';

class ApiService {
  private baseUrl: string;
  private token: string | null = null;

  constructor() {
    this.baseUrl = DEFAULT_BASE_URL;
  }

  setBaseUrl(url: string) {
    this.baseUrl = url;
  }

  setToken(token: string) {
    this.token = token;
  }

  clearToken() {
    this.token = null;
  }

  private async request<T>(
    endpoint: string,
    options: RequestInit = {}
  ): Promise<T> {
    const url = `${this.baseUrl}${endpoint}`;
    
    const headers: HeadersInit = {
      'Content-Type': 'application/json',
      ...options.headers,
    };

    if (this.token) {
      (headers as Record<string, string>)['Authorization'] = `Bearer ${this.token}`;
    }

    const response = await fetch(url, {
      ...options,
      headers,
    });

    if (!response.ok) {
      const error = await response.json().catch(() => ({}));
      throw new Error(error.message || `HTTP error! status: ${response.status}`);
    }

    return response.json();
  }

  // Auth endpoints
  async login(email: string, password: string) {
    return this.request<{token: string; user: any}>('/auth/login', {
      method: 'POST',
      body: JSON.stringify({email, password}),
    });
  }

  async logout() {
    return this.request('/auth/logout', {method: 'POST'});
  }

  // User endpoints
  async getProfile() {
    return this.request<any>('/users/profile');
  }

  async updateProfile(data: any) {
    return this.request('/users/profile', {
      method: 'PUT',
      body: JSON.stringify(data),
    });
  }

  // Robot endpoints
  async getRobots() {
    return this.request<any[]>('/robots');
  }

  async getRobot(id: string) {
    return this.request<any>(`/robots/${id}`);
  }

  async updateRobot(id: string, data: any) {
    return this.request(`/robots/${id}`, {
      method: 'PUT',
      body: JSON.stringify(data),
    });
  }

  // Task endpoints
  async getTasks() {
    return this.request<any[]>('/tasks');
  }

  async createTask(data: any) {
    return this.request('/tasks', {
      method: 'POST',
      body: JSON.stringify(data),
    });
  }

  async updateTask(id: string, data: any) {
    return this.request(`/tasks/${id}`, {
      method: 'PUT',
      body: JSON.stringify(data),
    });
  }

  // Sterilization history endpoints
  async getSterilizationHistory() {
    return this.request<any[]>('/sterilization/history');
  }

  async getSterilizationSession(id: string) {
    return this.request<any>(`/sterilization/history/${id}`);
  }

  // Alerts endpoints
  async getAlerts() {
    return this.request<any[]>('/alerts');
  }

  async resolveAlert(id: string, note: string) {
    return this.request(`/alerts/${id}/resolve`, {
      method: 'POST',
      body: JSON.stringify({note}),
    });
  }

  // Spaces/Locations endpoints
  async getSpaces() {
    return this.request<any[]>('/spaces');
  }

  async createSpace(data: any) {
    return this.request('/spaces', {
      method: 'POST',
      body: JSON.stringify(data),
    });
  }
}

export const api = new ApiService();
export default api;


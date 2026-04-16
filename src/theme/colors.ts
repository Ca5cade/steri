/**
 * Color palette matching v1 web application
 * Primary gradient: linear-gradient(90deg, #0A3F4C, #0C6980)
 */

export const colors = {
  // Primary Teal Colors (from v1)
  primary: {
    dark: '#0A3F4C',
    main: '#0C6980',
    light: '#14B8A6',
    50: '#F0FDFA',
    100: '#CCFBF1',
    200: '#99F6E4',
    600: '#0D9488',
    700: '#0F766E',
    800: '#115E59',
  },

  // Slate/Dark Colors (for cards like in dashboard)
  slate: {
    700: '#334155',
    800: '#1E293B',
    900: '#0F172A',
  },

  // Background Colors
  background: {
    main: '#F0FDFA', // teal-50
    white: '#FFFFFF',
    card: '#FFFFFF',
  },

  // Text Colors
  text: {
    primary: '#115E59', // teal-800
    secondary: '#5EEAD4', // teal-200
    light: '#99F6E4',
    white: '#FFFFFF',
    muted: '#6B7280', // gray-500
    dark: '#1F2937', // gray-800
  },

  // Status Colors
  status: {
    success: '#22C55E',
    successLight: '#DCFCE7',
    warning: '#F59E0B',
    warningLight: '#FEF3C7',
    error: '#EF4444',
    errorLight: '#FEE2E2',
    info: '#3B82F6',
    infoLight: '#DBEAFE',
  },

  // Badge Colors
  badge: {
    green: { bg: '#DCFCE7', text: '#166534' },
    blue: { bg: '#DBEAFE', text: '#1E40AF' },
    orange: { bg: '#FED7AA', text: '#C2410C' },
    purple: { bg: '#E9D5FF', text: '#7C3AED' },
    red: { bg: '#FEE2E2', text: '#DC2626' },
    pink: { bg: '#FCE7F3', text: '#DB2777' },
    yellow: { bg: '#FEF3C7', text: '#D97706' },
  },

  // Chart Colors (from bacteria data in v1)
  chart: {
    blue: '#3B82F6',
    purple: '#8B5CF6',
    green: '#22C55E',
    orange: '#F97316',
  },

  // Border Colors
  border: {
    light: '#E5E7EB',
    teal: '#99F6E4',
  },

  // Gradient definitions for LinearGradient
  gradients: {
    primary: ['#0A3F4C', '#0C6980'] as const,
    primaryReverse: ['#0C6980', '#0A3F4C'] as const,
  },
};

export default colors;


import type { CapacitorConfig } from '@capacitor/cli';

const config: CapacitorConfig = {
  appId: 'com.nlagravinese.newvegaswebmonitor',
  appName: 'NewVegasWebMonitor',
  webDir: 'dist',
  android: {
    allowMixedContent: true,
    edgeToEdge: true,
  },
  server: {
    androidScheme: 'http',
    cleartext: true,
  },
};

export default config;

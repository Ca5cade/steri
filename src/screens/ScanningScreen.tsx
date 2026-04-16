import React, { useState, useEffect, useRef } from 'react';
import {
    View,
    Text,
    StyleSheet,
    Animated,
    Easing,
    TouchableOpacity,
    Modal,
    TextInput,
    ScrollView,
} from 'react-native';
import { Feather as Icon } from '@expo/vector-icons';
import { useNavigation } from '@react-navigation/native';
import { NativeStackNavigationProp } from '@react-navigation/native-stack';
import { RootStackParamList } from '../navigation/types';
import { colors, spacing, fontSize, fontWeight, borderRadius } from '../theme';
import { useRobotStore } from '../store/robotStore';
import { Button } from '../components/ui';
import { useLanguageStore } from '../store/languageStore';
import { LanguageSwitcher } from '../components/LanguageSwitcher';

type ScanningScreenNavigationProp = NativeStackNavigationProp<RootStackParamList, 'Scanning'>;

interface DiscoveredDevice {
    ip: string;
    hostname: string;
}

export const ScanningScreen: React.FC = () => {
    const navigation = useNavigation<ScanningScreenNavigationProp>();
    const { t } = useLanguageStore();
    const [showManualIP, setShowManualIP] = useState(false);
    const [manualIP, setManualIP] = useState('192.168.1.100');
    const [discoveredDevices, setDiscoveredDevices] = useState<DiscoveredDevice[]>([]);
    const [scanningProgress, setScanningProgress] = useState(0);
    const [isScanning, setIsScanning] = useState(true);

    const pulseAnim1 = useRef(new Animated.Value(0)).current;
    const pulseAnim2 = useRef(new Animated.Value(0)).current;

    useEffect(() => {
        startScanning();
        startAnimation();
    }, []);

    const startAnimation = () => {
        const createPulse = (anim: Animated.Value, delay: number) => {
            return Animated.loop(
                Animated.sequence([
                    Animated.delay(delay),
                    Animated.timing(anim, {
                        toValue: 1,
                        duration: 2000,
                        easing: Easing.out(Easing.ease),
                        useNativeDriver: true,
                    }),
                    Animated.timing(anim, {
                        toValue: 0,
                        duration: 0,
                        useNativeDriver: true,
                    })
                ])
            );
        };

        Animated.parallel([
            createPulse(pulseAnim1, 0),
            createPulse(pulseAnim2, 1000),
        ]).start();
    };

    const testDevice = async (ip: string): Promise<DiscoveredDevice | null> => {
        try {
            const { setHost, connect } = useRobotStore.getState();
            setHost(ip);

            const success = await Promise.race([
                connect(),
                new Promise<boolean>((resolve) => setTimeout(() => resolve(false), 1000))
            ]);

            if (success) {
                const { api } = useRobotStore.getState();
                if (api) {
                    const { data: hostnameData } = await Promise.race([
                        api.getMachineHostname(),
                        new Promise<any>((resolve) => setTimeout(() => resolve({ data: null }), 1000))
                    ]);

                    if (hostnameData && hostnameData.hostname) {
                        return { ip, hostname: hostnameData.hostname };
                    }
                }
            }
        } catch (error) {
            // Device not responding
        }
        return null;
    };

    const startScanning = async () => {
        const targetHostname = "DESKTOP-P9IKV7I";
        const priorityIP = "192.168.100.116";

        // 1. Check priority IP first
        console.log(`Checking priority IP: ${priorityIP}`);
        const priorityDevice = await testDevice(priorityIP);

        if (priorityDevice) {
            console.log(`Found priority device at ${priorityIP}`);
            setDiscoveredDevices([priorityDevice]);

            const { setHost, connect } = useRobotStore.getState();
            setHost(priorityDevice.ip);
            const success = await connect();

            if (success) {
                setIsScanning(false);
                navigation.replace('Main');
                return; // Stop scanning
            }
        }

        // 2. Continue with normal scan if priority IP not found or failed to connect
        const baseIP = "192.168.1.";
        const devices: DiscoveredDevice[] = [];

        // Scan in batches of 10 for better performance
        const batchSize = 10;
        for (let start = 1; start <= 255; start += batchSize) {
            // Skip the priority IP if we already checked it
            const batch = [];

            for (let i = start; i < start + batchSize && i <= 255; i++) {
                const testIP = `${baseIP}${i}`;
                if (testIP === priorityIP) continue; // Skip already checked IP
                batch.push(testDevice(testIP));
            }

            const results = await Promise.all(batch);

            results.forEach(device => {
                if (device) {
                    devices.push(device);
                    setDiscoveredDevices(prev => [...prev, device]);

                    if (device.hostname === targetHostname) {
                        const { setHost, connect } = useRobotStore.getState();
                        setHost(device.ip);
                        connect().then(() => {
                            setIsScanning(false);
                            navigation.replace('Main');
                        });
                    }
                }
            });

            setScanningProgress(Math.round((start + batchSize) / 255 * 100));
        }

        setIsScanning(false);
    };

    const handleManualConnect = async () => {
        const { setHost, connect } = useRobotStore.getState();
        setHost(manualIP);
        const success = await connect();

        if (success) {
            setShowManualIP(false);
            navigation.replace('Main');
        } else {
            alert(`${t.scanning.failedConnect} ${manualIP}`);
        }
    };



    return (
        <View style={styles.container}>
            <View style={styles.header}>
                <View style={styles.headerTitleContainer}>
                    <Icon name="wind" size={24} color={colors.text.white} />
                    <View style={styles.divider} />
                    <Text style={styles.headerTitle}>{t.scanning.title}</Text>
                </View>
                <LanguageSwitcher />
            </View>

            <View style={styles.content}>
                <View style={styles.scanContainer}>
                    <View style={styles.robotIconContainer}>
                        <Icon name="cpu" size={32} color={colors.text.white} />
                    </View>

                    <View style={styles.wifiIconContainer}>
                        <Icon name="wifi" size={48} color={colors.text.white} />
                    </View>

                    <Animated.View style={[styles.pulseCircle, {
                        transform: [{ scale: pulseAnim1 }],
                        opacity: pulseAnim1.interpolate({
                            inputRange: [0, 1],
                            outputRange: [0.8, 0]
                        })
                    }]} />
                    <Animated.View style={[styles.pulseCircle, {
                        transform: [{ scale: pulseAnim2 }],
                        opacity: pulseAnim2.interpolate({
                            inputRange: [0, 1],
                            outputRange: [0.8, 0]
                        })
                    }]} />
                </View>

                <Text style={styles.statusText}>
                    {isScanning ? `${t.scanning.scanning} ${scanningProgress}%` : t.scanning.scanComplete}
                </Text>

                {discoveredDevices.length > 0 && (
                    <View style={styles.devicesContainer}>
                        <Text style={styles.devicesTitle}>{t.scanning.discoveredDevices}</Text>
                        <ScrollView style={styles.devicesList}>
                            {discoveredDevices.map((device, index) => (
                                <View key={index} style={styles.deviceItem}>
                                    <Text style={styles.deviceHostname}>{device.hostname}</Text>
                                    <Text style={styles.deviceIP}>{device.ip}</Text>
                                </View>
                            ))}
                        </ScrollView>
                    </View>
                )}

                <TouchableOpacity
                    style={styles.manualButton}
                    onPress={() => setShowManualIP(true)}>
                    <Icon name="edit" size={18} color={colors.text.white} />
                    <Text style={styles.manualButtonText}>{t.scanning.manualEntry}</Text>
                </TouchableOpacity>
            </View>

            <Modal
                visible={showManualIP}
                transparent
                animationType="fade"
                onRequestClose={() => setShowManualIP(false)}>
                <View style={styles.modalOverlay}>
                    <View style={styles.manualIPModal}>
                        <Text style={styles.modalTitle}>{t.scanning.manualTitle}</Text>
                        <TextInput
                            style={styles.ipInput}
                            value={manualIP}
                            onChangeText={setManualIP}
                            placeholder="192.168.1.100"
                            placeholderTextColor="rgba(255,255,255,0.5)"
                            keyboardType="numeric"
                        />
                        <View style={styles.modalButtons}>
                            <Button
                                variant="outline"
                                onPress={() => setShowManualIP(false)}
                                style={styles.modalButton}>
                                {t.common.cancel}
                            </Button>
                            <Button
                                variant="primary"
                                onPress={handleManualConnect}
                                style={styles.modalButton}>
                                {t.common.connect}
                            </Button>
                        </View>
                    </View>
                </View>
            </Modal>


        </View>
    );
};

const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: '#0A3F4C',
    },
    header: {
        flexDirection: 'row',
        alignItems: 'center',
        justifyContent: 'space-between',
        paddingHorizontal: spacing.lg,
        paddingVertical: spacing.xl,
    },

    headerTitleContainer: {
        flexDirection: 'row',
        alignItems: 'center',
        gap: spacing.md,
    },
    divider: {
        width: 1,
        height: 24,
        backgroundColor: 'rgba(255,255,255,0.3)',
    },
    headerTitle: {
        fontSize: fontSize.md,
        color: colors.text.white,
        fontWeight: fontWeight.medium,
    },
    langButton: {
        padding: spacing.sm,
    },
    content: {
        flex: 1,
        alignItems: 'center',
        justifyContent: 'center',
        paddingHorizontal: spacing.lg,
    },
    scanContainer: {
        width: 300,
        height: 300,
        alignItems: 'center',
        justifyContent: 'center',
        marginBottom: spacing.xl,
    },
    robotIconContainer: {
        position: 'absolute',
        top: 0,
        zIndex: 10,
        width: 64,
        height: 64,
        borderRadius: 32,
        backgroundColor: 'rgba(255,255,255,0.1)',
        alignItems: 'center',
        justifyContent: 'center',
    },
    wifiIconContainer: {
        zIndex: 10,
        width: 96,
        height: 96,
        borderRadius: 48,
        backgroundColor: '#2D7D90',
        alignItems: 'center',
        justifyContent: 'center',
    },
    pulseCircle: {
        position: 'absolute',
        width: 300,
        height: 300,
        borderRadius: 150,
        backgroundColor: 'rgba(45, 125, 144, 0.3)',
    },
    statusText: {
        color: colors.text.white,
        fontSize: fontSize.lg,
        fontWeight: fontWeight.bold,
        marginBottom: spacing.lg,
    },
    devicesContainer: {
        width: '100%',
        maxHeight: 200,
        marginBottom: spacing.lg,
    },
    devicesTitle: {
        color: colors.text.white,
        fontSize: fontSize.md,
        fontWeight: fontWeight.bold,
        marginBottom: spacing.sm,
    },
    devicesList: {
        width: '100%',
    },
    deviceItem: {
        backgroundColor: 'rgba(255,255,255,0.1)',
        padding: spacing.md,
        borderRadius: borderRadius.md,
        marginBottom: spacing.sm,
    },
    deviceHostname: {
        color: colors.text.white,
        fontSize: fontSize.md,
        fontWeight: fontWeight.bold,
    },
    deviceIP: {
        color: 'rgba(255,255,255,0.7)',
        fontSize: fontSize.sm,
        marginTop: 4,
    },
    manualButton: {
        flexDirection: 'row',
        alignItems: 'center',
        justifyContent: 'center',
        gap: spacing.sm,
        backgroundColor: colors.primary.main, // Dominant color
        paddingHorizontal: spacing.xl,
        paddingVertical: spacing.md,
        borderRadius: borderRadius.full,
        width: '80%', // Wider button
        shadowColor: "#000",
        shadowOffset: {
            width: 0,
            height: 2,
        },
        shadowOpacity: 0.25,
        shadowRadius: 3.84,
        elevation: 5,
    },
    manualButtonText: {
        color: colors.text.white,
        fontSize: fontSize.lg, // Larger text
        fontWeight: fontWeight.bold,
    },
    modalOverlay: {
        flex: 1,
        backgroundColor: 'rgba(0,0,0,0.7)',
        justifyContent: 'center',
        alignItems: 'center',
    },
    manualIPModal: {
        backgroundColor: '#0A3F4C',
        borderRadius: borderRadius.lg,
        padding: spacing.xl,
        width: '80%',
        maxWidth: 400,
    },
    modalTitle: {
        color: colors.text.white,
        fontSize: fontSize.xl,
        fontWeight: fontWeight.bold,
        marginBottom: spacing.lg,
    },
    ipInput: {
        backgroundColor: 'rgba(255,255,255,0.1)',
        borderRadius: borderRadius.md,
        padding: spacing.md,
        color: colors.text.white,
        fontSize: fontSize.md,
        marginBottom: spacing.lg,
    },
    modalButtons: {
        flexDirection: 'row',
        gap: spacing.md,
    },
    modalButton: {
        flex: 1,
    },

});

export default ScanningScreen;

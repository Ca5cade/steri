import React, { useState, useEffect } from 'react';
import {
    View,
    Text,
    StyleSheet,
    Image,
    TouchableOpacity,
    Modal,
} from 'react-native';
import { SafeAreaView } from 'react-native-safe-area-context';
import { Feather as Icon } from '@expo/vector-icons';
import { useNavigation } from '@react-navigation/native';
import { MainTabParamList } from '../navigation/types';
import { BottomTabNavigationProp } from '@react-navigation/bottom-tabs';
import { Button, Modal as UIModal } from '../components/ui';
import { colors, spacing, fontSize, fontWeight, borderRadius } from '../theme';
import { useRobotStore } from '../store/robotStore';
import { useLanguageStore } from '../store/languageStore';

type RobotStatusScreenNavigationProp = BottomTabNavigationProp<MainTabParamList, 'Home'>;

import { SoundService } from '../services/SoundService';

import { LanguageSwitcher } from '../components/LanguageSwitcher';

export const RobotStatusScreen: React.FC = () => {
    const navigation = useNavigation<RobotStatusScreenNavigationProp>();
    const { t } = useLanguageStore();
    const { hostname, batteryLevel, maxSpeed, currentMode, updateStatus, setNavigationMaximumSpeed, startDocking } = useRobotStore();
    const [speed, setSpeed] = useState(0.6);
    const [showSpeedModal, setShowSpeedModal] = useState(false);
    const [showSidebar, setShowSidebar] = useState(false);

    // Update status periodically
    useEffect(() => {
        SoundService.speak(t.robotStatus.connected);
        updateStatus(); // Initial fetch
        const interval = setInterval(() => {
            updateStatus();
        }, 2000); // Update every 2 seconds
        return () => clearInterval(interval);
    }, []);

    // Sync local speed state with maxSpeed from store
    useEffect(() => {
        setSpeed(maxSpeed);
    }, [maxSpeed]);



    const handleDocking = async () => {
        SoundService.speak(t.robotStatus.lookingForDock);
        await startDocking();
    };

    const handleStartScan = () => {
        SoundService.speak(t.robotStatus.startingMapping);
        // Navigate to the Map tab
        // @ts-ignore - Navigate to nested tab
        navigation.navigate('Map');
    };

    const handleSpeedChange = async () => {
        await setNavigationMaximumSpeed(speed);
        setShowSpeedModal(false);
        updateStatus(); // Refresh status
    };

    // Determine status based on mode
    const statusText = currentMode === 0 ? t.robotStatus.idle : t.robotStatus.active;

    return (
        <SafeAreaView style={styles.container}>
            {/* Header */}
            <View style={styles.header}>
                <View style={styles.headerTitleContainer}>
                    <Text style={styles.headerTitle}>{t.robotStatus.title}</Text>
                    <Icon name="cpu" size={20} color={colors.text.white} style={{ marginLeft: 4 }} />
                </View>
                <View style={{ flexDirection: 'row', alignItems: 'center', gap: 10 }}>
                    <View style={styles.connectedBadge}>
                        <View style={styles.connectedDot} />
                        <Text style={styles.connectedText}>{t.robotStatus.connected}</Text>
                    </View>
                    <LanguageSwitcher />
                </View>
            </View>

            <View style={styles.content}>
                {/* Robot Image */}
                <View style={styles.imageContainer}>
                    <Image
                        source={require('../../assets/robot.png')}
                        style={styles.robotImage}
                        resizeMode="contain"
                    />
                </View>

                {/* Robot Info */}
                <View style={styles.infoContainer}>
                    <Text style={styles.robotModel}>{hostname || t.common.loading}</Text>
                    <Text style={styles.robotSubtitle}>{t.robotStatus.subtitle}</Text>

                    <View style={styles.statsRow}>
                        <View style={styles.statItem}>
                            <Icon name="battery-charging" size={16} color={colors.text.white} />
                            <Text style={styles.statText}>{batteryLevel}%</Text>
                        </View>

                        <TouchableOpacity onPress={() => setShowSpeedModal(true)} style={styles.statItem}>
                            <Icon name="activity" size={16} color={colors.text.white} />
                            <Text style={styles.statText}>{maxSpeed.toFixed(1)} m/s</Text>
                        </TouchableOpacity>

                        <View style={styles.statItem}>
                            <Text style={styles.statLabel}>{t.robotStatus.status}</Text>
                            <View style={styles.statusDot} />
                            <Text style={styles.statText}>{statusText}</Text>
                        </View>
                    </View>
                </View>

                {/* Action Button */}
                <Button
                    variant="primary"
                    size="lg"
                    style={styles.actionButton}
                    onPress={handleStartScan}
                >
                    {t.robotStatus.startScan}
                </Button>

                <Button
                    variant="outline"
                    size="lg"
                    style={[styles.actionButton, { marginTop: spacing.md }]}
                    onPress={handleDocking}
                    textStyle={{ color: colors.text.white }}
                >
                    {t.robotStatus.dockingCharge}
                </Button>
            </View>

            {/* Speed Adjustment Modal */}
            <UIModal
                visible={showSpeedModal}
                onClose={() => setShowSpeedModal(false)}
                title={t.robotStatus.adjustSpeed}>
                <View style={styles.modalContent}>
                    <Text style={styles.speedValue}>{speed.toFixed(1)} m/s</Text>
                    <View style={styles.sliderContainer}>
                        <TouchableOpacity onPress={() => setSpeed(Math.max(0.3, speed - 0.1))} style={styles.adjustBtn}>
                            <Icon name="minus" size={24} color={colors.primary.main} />
                        </TouchableOpacity>
                        <View style={styles.sliderBar}>
                            <View style={[styles.sliderFill, { width: `${((speed - 0.3) / 0.7) * 100}%` }]} />
                        </View>
                        <TouchableOpacity onPress={() => setSpeed(Math.min(1.0, speed + 0.1))} style={styles.adjustBtn}>
                            <Icon name="plus" size={24} color={colors.primary.main} />
                        </TouchableOpacity>
                    </View>
                    <Button variant="primary" fullWidth onPress={handleSpeedChange}>
                        {t.common.confirm}
                    </Button>
                </View>
            </UIModal>


        </SafeAreaView >
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
        paddingVertical: spacing.md,
    },

    headerTitleContainer: {
        flexDirection: 'row',
        alignItems: 'center',
        gap: spacing.sm,
    },
    headerLogo: {
        width: 24,
        height: 24,
        tintColor: colors.text.white,
    },
    headerTitle: {
        fontSize: fontSize.lg,
        fontWeight: fontWeight.bold,
        color: colors.text.white,
    },
    connectedBadge: {
        flexDirection: 'row',
        alignItems: 'center',
        backgroundColor: 'rgba(255,255,255,0.1)',
        paddingHorizontal: spacing.md,
        paddingVertical: spacing.xs,
        borderRadius: borderRadius.full,
        gap: spacing.xs,
    },
    connectedDot: {
        width: 8,
        height: 8,
        borderRadius: 4,
        backgroundColor: colors.status.success,
    },
    connectedText: {
        color: colors.status.success,
        fontSize: fontSize.xs,
        fontWeight: fontWeight.bold,
    },
    content: {
        flex: 1,
        alignItems: 'center',
        justifyContent: 'center',
        paddingBottom: spacing['4xl'],
    },
    imageContainer: {
        width: '80%',
        height: '50%',
        justifyContent: 'center',
        alignItems: 'center',
    },
    robotImage: {
        width: '100%',
        height: '100%',
    },
    infoContainer: {
        alignItems: 'center',
        marginBottom: spacing.xl,
    },
    robotModel: {
        fontSize: fontSize['2xl'],
        fontWeight: fontWeight.bold,
        color: colors.text.white,
        marginBottom: spacing.xs,
    },
    robotSubtitle: {
        fontSize: fontSize.sm,
        color: 'rgba(255,255,255,0.7)',
        marginBottom: spacing.lg,
    },
    statsRow: {
        flexDirection: 'row',
        alignItems: 'center',
        gap: spacing.xl,
    },
    statItem: {
        flexDirection: 'row',
        alignItems: 'center',
        gap: spacing.xs,
    },
    statText: {
        color: colors.text.white,
        fontWeight: fontWeight.medium,
    },
    statLabel: {
        color: 'rgba(255,255,255,0.7)',
        marginRight: 4,
    },
    statusDot: {
        width: 8,
        height: 8,
        borderRadius: 4,
        backgroundColor: colors.status.success,
    },
    actionButton: {
        width: 200,
        backgroundColor: '#0C6980',
    },
    modalContent: {
        alignItems: 'center',
        gap: spacing.lg,
    },
    speedValue: {
        fontSize: fontSize['3xl'],
        fontWeight: fontWeight.bold,
        color: colors.text.primary,
    },
    sliderContainer: {
        flexDirection: 'row',
        alignItems: 'center',
        width: '100%',
        gap: spacing.md,
    },
    adjustBtn: {
        padding: spacing.sm,
        backgroundColor: colors.background.card,
        borderRadius: borderRadius.full,
    },
    sliderBar: {
        flex: 1,
        height: 6,
        backgroundColor: colors.border.light,
        borderRadius: 3,
        overflow: 'hidden',
    },
    sliderFill: {
        height: '100%',
        backgroundColor: colors.primary.main,
    },

});

export default RobotStatusScreen;

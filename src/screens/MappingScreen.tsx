import React, { useState, useEffect, useRef, useCallback } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert, ScrollView, useWindowDimensions, Platform, Modal } from 'react-native';
import { useRobotStore } from '../store/robotStore';
import { AutonomousMappingService } from '../services/AutonomousMappingService';
import { SafeAreaView } from 'react-native-safe-area-context';
import { useNavigation } from '@react-navigation/native';
import { Feather as Icon } from '@expo/vector-icons';
import { Joystick } from '../components/ui/Joystick';
import MapDisplay from '../components/MapDisplay';
import { colors, spacing, fontSize, fontWeight, borderRadius } from '../theme';
import { Button, Input } from '../components/ui';
import { SoundService } from '../services/SoundService';
import { database } from '../config/firebase';
import { ref, push, set, onValue, update, remove } from 'firebase/database';
import { useLanguageStore } from '../store/languageStore';
import { LanguageSwitcher } from '../components/LanguageSwitcher';


const MappingScreen = () => {
    // Selective selectors to prevent re-renders on unrelated store changes
    const api = useRobotStore(state => state.api);
    const { t } = useLanguageStore();
    const isConnected = useRobotStore(state => state.isConnected);
    const robotPose = useRobotStore(state => state.robotPose);
    const updateStatus = useRobotStore(state => state.updateStatus);
    const batteryLevel = useRobotStore(state => state.batteryLevel);
    const maxSpeed = useRobotStore(state => state.maxSpeed);
    const setNavigationMaximumSpeed = useRobotStore(state => state.setNavigationMaximumSpeed);

    const [mappingService, setMappingService] = useState<AutonomousMappingService | null>(null);
    const [isMapping, setIsMapping] = useState(false);
    const [isManualMapping, setIsManualMapping] = useState(false);
    const [mapRotation, setMapRotation] = useState(0);
    const navigation = useNavigation();
    const [showSpeedModal, setShowSpeedModal] = useState(false);
    const [speed, setSpeed] = useState(0.6);

    const { width, height } = useWindowDimensions();
    const isLandscape = width > height;

    // Map state
    const [mapImage, setMapImage] = useState<string | null>(null);
    const [mapResolution, setMapResolution] = useState(0.05);
    const [mapOrigin, setMapOrigin] = useState({ x: 0, y: 0 });
    const [mapHeight, setMapHeight] = useState(0);

    // Firebase Maps State
    const [savedMaps, setSavedMaps] = useState<any[]>([]);
    const [selectedMapId, setSelectedMapId] = useState<string | null>(null);
    const [showRenameModal, setShowRenameModal] = useState(false);
    const [newMapName, setNewMapName] = useState('');
    const [renamingMapId, setRenamingMapId] = useState<string | null>(null);
    const [isViewingSavedMap, setIsViewingSavedMap] = useState(false);

    const hostname = useRobotStore(state => state.hostname);



    // Throttling for joystick - Reeman requires commands every 300ms
    const THROTTLE_MS = 300;

    // Continuous command sending for joystick
    const joystickInterval = useRef<NodeJS.Timeout | null>(null);
    const currentJoystickData = useRef<{ vx: number; vth: number }>({ vx: 0, vth: 0 });

    useEffect(() => {
        if (api) {
            setMappingService(new AutonomousMappingService(api));
        }
    }, [api]);

    const fetchMap = useCallback(async () => {
        if (!api || showRenameModal || isViewingSavedMap) return;
        const { data, error } = await api.getCurrentMap();
        if (!error && data && data.image_url) {
            // Only update if changed to prevent render loops
            setMapImage(prev => prev !== data.image_url ? data.image_url : prev);

            if (data.resolution) {
                setMapResolution(prev => prev !== data.resolution ? data.resolution : prev);
            }

            if (data.origin_x !== undefined && data.origin_y !== undefined) {
                setMapOrigin(prev => {
                    if (prev.x !== data.origin_x || prev.y !== data.origin_y) {
                        return { x: data.origin_x, y: data.origin_y };
                    }
                    return prev;
                });
            }

            if (data.height) {
                setMapHeight(prev => prev !== data.height ? data.height : prev);
            }
        }
    }, [api, showRenameModal, isViewingSavedMap]);

    // Polling for map and status
    useEffect(() => {
        const interval = setInterval(() => {
            fetchMap();
            updateStatus();
        }, 500);
        return () => clearInterval(interval);
    }, [fetchMap, updateStatus]);

    // Sync local speed state with maxSpeed from store
    useEffect(() => {
        // Only update local speed if it's significantly different to avoid loops
        if (maxSpeed !== undefined && Math.abs(speed - maxSpeed) > 0.01) {
            setSpeed(maxSpeed);
        }
    }, [maxSpeed]);

    // Cleanup joystick interval on unmount
    useEffect(() => {
        return () => {
            if (joystickInterval.current) {
                clearInterval(joystickInterval.current);
            }
        };
    }, []);

    const handleStartAutonomousMapping = async () => {
        if (!mappingService) return;

        // Reset local map state for a "blank" start
        setMapImage(null);
        setMapResolution(0.05); // Default
        setMapOrigin({ x: 0, y: 0 }); // Default

        SoundService.speak(t.mapping.startingAuto);
        setIsMapping(true);
        setIsManualMapping(false);
        mappingService.start();
    };



    const handleStartManualMapping = async () => {
        if (!api) {
            Alert.alert(t.common.error, t.mapping.errorNotConnected);
            return;
        }

        // Reset local map state for a "blank" start
        setMapImage(null);
        setMapResolution(0.05); // Default
        setMapOrigin({ x: 0, y: 0 }); // Default

        // Force reset by switching to Idle (0) first
        await api.switchMappingMode(0);
        await new Promise(resolve => setTimeout(resolve, 1000));

        const { error } = await api.switchMappingMode(1);
        if (error) {
            Alert.alert(t.common.error, "Failed to switch to mapping mode: " + error);
            return;
        }
        SoundService.speak(t.mapping.scanningArea);
        setIsMapping(true);
        setIsManualMapping(true);
    };

    const handleStopMapping = async () => {
        if (mappingService && !isManualMapping) {
            mappingService.stop();
        } else if (api && isManualMapping) {
            await api.switchMappingMode(0);
        }
        setIsMapping(false);
        setIsManualMapping(false);
    };

    // Listen for saved maps
    useEffect(() => {
        if (!hostname) return;
        const mapsRef = ref(database, `${hostname}/maps`);
        const unsubscribe = onValue(mapsRef, (snapshot) => {
            const data = snapshot.val();
            if (data) {
                const mapsList = Object.entries(data).map(([id, map]: [string, any]) => ({
                    id,
                    ...map,
                }));
                setSavedMaps(mapsList);
            } else {
                setSavedMaps([]);
            }
        });

        return () => unsubscribe();
    }, [hostname]);

    const handleSaveMap = async () => {
        if (!api) return;

        // Save to Robot
        const { error } = await api.saveMap();
        if (error) {
            Alert.alert('Error', 'Failed to save map on robot: ' + error);
            return;
        }

        // Save to Firebase
        if (hostname && mapImage) {
            try {
                // Fetch image to get base64 if it's a URL, or use as is if already base64
                // Assuming mapImage is a URL from the robot, we need to fetch it
                // Note: If mapImage is already base64, this fetch might fail or be unnecessary.
                // Reeman API usually returns a URL.

                let base64Image = mapImage;
                if (mapImage.startsWith('http')) {
                    const response = await fetch(mapImage);
                    const blob = await response.blob();
                    base64Image = await new Promise((resolve, reject) => {
                        const reader = new FileReader();
                        reader.onloadend = () => resolve(reader.result as string);
                        reader.onerror = reject;
                        reader.readAsDataURL(blob);
                    });
                }

                const mapsRef = ref(database, `${hostname}/maps`);
                const newMapRef = push(mapsRef);
                await set(newMapRef, {
                    name: `Map ${new Date().toLocaleString()}`,
                    image: base64Image,
                    timestamp: Date.now(),
                });
                SoundService.speak(t.mapping.mapSaved);
                Alert.alert(t.common.success, t.mapping.successMapSaved);
            } catch (firebaseError: any) {
                console.error("Firebase save error", firebaseError);
                Alert.alert('Warning', 'Map saved to robot but failed to save to database: ' + firebaseError.message);
            }
        } else {
            SoundService.speak(t.mapping.mapSaved);
            Alert.alert(t.common.success, 'Map saved to robot (Database skipped: No hostname or image).');
        }
    };

    const handleRenameMap = async () => {
        if (!renamingMapId || !newMapName.trim() || !hostname) return;

        try {
            const mapRef = ref(database, `${hostname}/maps/${renamingMapId}`);
            await update(mapRef, { name: newMapName });
            setShowRenameModal(false);
            setNewMapName('');
            setRenamingMapId(null);
        } catch (error: any) {
            Alert.alert('Error', 'Failed to rename map: ' + error.message);
        }
    };

    const handleDeleteMap = async (id?: string) => {
        const mapId = typeof id === 'string' ? id : renamingMapId;
        if (!mapId || !hostname) return;

        try {
            const mapRef = ref(database, `${hostname}/maps/${mapId}`);
            await remove(mapRef);
            if (mapId === renamingMapId) {
                setShowRenameModal(false);
                setNewMapName('');
                setRenamingMapId(null);
            }
        } catch (error: any) {
            Alert.alert('Error', 'Failed to delete map: ' + error.message);
        }
    };

    const handleReturnToLive = () => {
        setIsViewingSavedMap(false);
        // fetchMap will resume automatically on next interval
    };

    const handleResetMap = async () => {
        // Implement reset map logic if available in API
        Alert.alert('Info', 'Reset Map functionality to be implemented');
    };



    const handleJoystickMove = (data: { x: number; y: number; angle: number; intensity: number }) => {
        if (!api) return;

        // Convert joystick input to robot speeds
        const MAX_LINEAR_SPEED = 0.5;  // m/s
        const MAX_ANGULAR_SPEED = 1.0; // rad/s - Increased for better rotation response

        let vx = 0;
        let vth = 0;

        // Exclusive movement: Prioritize the axis with greater magnitude
        if (Math.abs(data.y) > Math.abs(data.x)) {
            // Vertical movement (Forward/Backward)
            // Invert y because joystick up is negative
            vx = -data.y * data.intensity * MAX_LINEAR_SPEED;
        } else {
            // Horizontal movement (Rotation)
            // Use negative x for angular velocity (Left turn = positive vth)
            vth = -data.x * data.intensity * MAX_ANGULAR_SPEED;
        }

        console.log('Joystick:', { x: data.x, y: data.y, vx, vth });

        // Store current values
        currentJoystickData.current = { vx, vth };

        // Send command immediately
        api.setSpeed(vx, vth);

        // Start continuous sending if not already started
        if (!joystickInterval.current) {
            joystickInterval.current = setInterval(() => {
                const { vx: currentVx, vth: currentVth } = currentJoystickData.current;
                api.setSpeed(currentVx, currentVth);
            }, THROTTLE_MS);
        }
    };

    const handleJoystickStop = () => {
        // Clear interval
        if (joystickInterval.current) {
            clearInterval(joystickInterval.current);
            joystickInterval.current = null;
        }

        // Reset values
        currentJoystickData.current = { vx: 0, vth: 0 };

        // Send stop command
        if (api) {
            api.setSpeed(0, 0);
        }
    };

    const toggleRotation = () => {
        setMapRotation(prev => (prev + 90) % 360);
    };



    return (
        <View style={styles.container}>
            {/* Header */}
            <View style={styles.header}>
                <View style={styles.headerTitleContainer}>
                    <Icon name="wind" size={24} color={colors.text.white} />
                    <View style={styles.divider} />
                    <Text style={styles.headerTitle}>{t.mapping.title}</Text>
                </View>
                <LanguageSwitcher />
            </View>

            <View style={styles.mappingContent}>
                {/* Left Sidebar: Status & Params */}
                <View style={styles.mappingSidebar}>
                    {/* Robot Status */}
                    <View style={styles.statusSection}>
                        <Text style={styles.sectionTitle}>{t.mapping.robotStatus}</Text>
                        <View style={styles.statusRow}>
                            <Text style={styles.statusLabel}>{t.mapping.position}</Text>
                            <Text style={styles.statusValue}>
                                X: {robotPose?.x?.toFixed(1) || '0.0'}, Y: {robotPose?.y?.toFixed(1) || '0.0'}
                            </Text>
                        </View>
                        <View style={styles.statusRow}>
                            <Text style={styles.statusLabel}>{t.mapping.battery}</Text>
                            <View style={styles.batteryWrapper}>
                                <View style={styles.batteryBar}>
                                    <View style={[styles.batteryFill, { width: `${batteryLevel}%` }]} />
                                </View>
                                <Text style={styles.batteryText}>{batteryLevel}%</Text>
                            </View>
                        </View>
                        <TouchableOpacity onPress={() => setShowSpeedModal(true)} style={styles.statusRow}>
                            <Text style={styles.statusLabel}>{t.mapping.speed}</Text>
                            <Text style={styles.statusValue}>{maxSpeed?.toFixed(1) || '0.6'} m/s</Text>
                        </TouchableOpacity>
                    </View>

                    <View style={styles.separator} />



                    {/* Actions */}
                    <View style={styles.actionButtons}>
                        {!isMapping ? (
                            <Button
                                variant="primary"
                                style={styles.stopButton}
                                onPress={handleStartManualMapping}
                                icon={<Icon name="play" size={16} color="white" />}
                            >
                                {t.mapping.startManual}
                            </Button>
                        ) : (
                            <Button
                                variant="primary"
                                style={[styles.stopButton, { backgroundColor: colors.status.error }]}
                                onPress={handleStopMapping}
                                icon={<Icon name="square" size={16} color="white" />}
                            >
                                {t.mapping.stopScan}
                            </Button>
                        )}

                        {!isMapping && (
                            <Button
                                variant="outline"
                                style={[styles.stopButton, { marginTop: spacing.sm, borderColor: colors.primary.main }]}
                                onPress={handleStartAutonomousMapping}
                                icon={<Icon name="cpu" size={16} color={colors.primary.main} />}
                                textStyle={{ color: colors.text.white }}
                            >
                                {t.mapping.startAuto}
                            </Button>
                        )}

                        <Button
                            variant="secondary"
                            style={[styles.resetButton, { marginTop: spacing.sm, backgroundColor: colors.status.success }]}
                            onPress={handleSaveMap}
                            icon={<Icon name="save" size={16} color="white" />}
                        >
                            {t.mapping.saveMap}
                        </Button>
                    </View>

                    {/* Saved Maps List */}
                    <View style={styles.savedMapsContainer}>
                        <Text style={styles.sectionTitle}>{t.mapping.savedMaps}</Text>
                        {!hostname ? (
                            <Text style={styles.emptyStateText}>{t.mapping.emptyMapConnect}</Text>
                        ) : savedMaps.length === 0 ? (
                            <Text style={styles.emptyStateText}>{t.mapping.emptyMapNoData}</Text>
                        ) : (
                            <ScrollView style={styles.mapsList} nestedScrollEnabled>
                                {savedMaps.map((map) => (
                                    <TouchableOpacity
                                        key={map.id}
                                        style={[styles.mapItem, renamingMapId === map.id && { backgroundColor: 'rgba(255,255,255,0.1)' }]}
                                        onPress={() => {
                                            // Load map only
                                            if (map.image) {
                                                setMapImage(map.image);
                                                setIsViewingSavedMap(true);
                                            }
                                        }}
                                    >
                                        <Text style={styles.mapName} numberOfLines={1}>{map.name}</Text>
                                        <View style={styles.mapActions}>
                                            <TouchableOpacity
                                                onPress={() => handleDeleteMap(map.id)}
                                                style={styles.iconButton}
                                            >
                                                <Icon name="trash-2" size={14} color={colors.status.error} />
                                            </TouchableOpacity>
                                            <TouchableOpacity
                                                onPress={() => {
                                                    setRenamingMapId(map.id);
                                                    setNewMapName(map.name);
                                                    setShowRenameModal(true);
                                                }}
                                                style={styles.iconButton}
                                            >
                                                <Icon name="edit-2" size={14} color={colors.text.muted} />
                                            </TouchableOpacity>
                                        </View>
                                    </TouchableOpacity>
                                ))}
                            </ScrollView>
                        )}
                    </View>
                </View>

                {/* Right Area: Map & Joystick */}
                <View style={styles.mapArea}>
                    <MapDisplay
                        mapImage={mapImage}
                        resolution={mapResolution}
                        originX={mapOrigin.x}
                        originY={mapOrigin.y}
                        mapHeight={mapHeight}
                        mapWidth={0}
                        robotPose={robotPose}
                        rotation={mapRotation}
                    />

                    {/* Floating Rotation Button */}
                    <TouchableOpacity style={styles.floatingButton} onPress={toggleRotation}>
                        <Icon name="rotate-cw" color={colors.text.white} size={22} />
                    </TouchableOpacity>

                    {/* Joystick Overlay - Always Visible */}
                    <View style={styles.joystickContainer}>
                        <Joystick
                            onMove={handleJoystickMove}
                            onStop={handleJoystickStop}
                            size={180}
                            color="#2A82DA"
                            outerColor="#505050"
                        />
                    </View>
                </View>
            </View>

            {/* Rename Map Modal - Custom Overlay */}
            {showRenameModal && (
                <View style={styles.customModalOverlay}>
                    <View style={styles.popupContainer}>
                        <View style={styles.popupHeader}>
                            <Text style={styles.modalTitle}>{t.mapping.renameTitle}</Text>
                            <TouchableOpacity onPress={() => handleDeleteMap()} style={styles.deleteIconButton}>
                                <Icon name="trash-2" size={20} color={colors.status.error} />
                            </TouchableOpacity>
                        </View>

                        <Input
                            value={newMapName}
                            onChangeText={setNewMapName}
                            placeholder={t.mapping.renamePlaceholder}
                            containerStyle={{ marginBottom: spacing.lg, width: '100%' }}
                        />

                        <View style={styles.modalButtons}>
                            <Button variant="outline" onPress={() => setShowRenameModal(false)} style={styles.modalButton}>
                                {t.common.cancel}
                            </Button>
                            <Button variant="primary" onPress={handleRenameMap} style={styles.modalButton}>
                                {t.common.save}
                            </Button>
                        </View>
                    </View>
                </View>
            )}

            {/* Speed Adjustment Modal */}
            <Modal
                visible={showSpeedModal}
                transparent
                animationType="fade"
                onRequestClose={() => setShowSpeedModal(false)}>
                <View style={styles.modalOverlay}>
                    <View style={styles.speedModal}>
                        <Text style={styles.modalTitle}>{t.robotStatus.adjustSpeed}</Text>
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
                        <View style={styles.modalButtons}>
                            <Button variant="outline" onPress={() => setShowSpeedModal(false)} style={styles.modalButton}>
                                {t.common.cancel}
                            </Button>
                            <Button variant="primary" onPress={async () => {
                                await setNavigationMaximumSpeed(speed);
                                setShowSpeedModal(false);
                                updateStatus();
                            }} style={styles.modalButton}>
                                {t.common.confirm}
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
    mappingContent: {
        flex: 1,
        flexDirection: 'row',
        padding: spacing.lg,
        gap: spacing.lg,
    },
    mappingSidebar: {
        width: 300,
        backgroundColor: 'rgba(0,0,0,0.2)',
        borderRadius: borderRadius.lg,
        padding: spacing.lg,
        flex: 1, // Fill available height
    },
    sectionTitle: {
        color: colors.text.white,
        fontSize: fontSize.md,
        fontWeight: fontWeight.bold,
        marginBottom: spacing.md,
        fontFamily: 'monospace',
    },
    statusSection: {
        marginBottom: spacing.lg,
    },
    statusRow: {
        flexDirection: 'row',
        justifyContent: 'space-between',
        alignItems: 'center',
        marginBottom: spacing.sm,
    },
    statusLabel: {
        color: colors.text.muted,
        fontSize: fontSize.sm,
        fontFamily: 'monospace',
    },
    statusValue: {
        color: colors.text.white,
        fontSize: fontSize.sm,
        fontFamily: 'monospace',
    },
    batteryWrapper: {
        flexDirection: 'row',
        alignItems: 'center',
        gap: spacing.sm,
    },
    batteryBar: {
        width: 60,
        height: 6,
        backgroundColor: 'rgba(255,255,255,0.1)',
        borderRadius: 3,
        overflow: 'hidden',
    },
    batteryFill: {
        height: '100%',
        backgroundColor: colors.status.success,
    },
    batteryText: {
        color: colors.status.success,
        fontSize: fontSize.sm,
        fontWeight: 'bold',
    },
    separator: {
        height: 1,
        backgroundColor: 'rgba(255,255,255,0.1)',
        marginVertical: spacing.lg,
    },

    actionButtons: {
        gap: spacing.md,
    },
    stopButton: {
        width: '100%',
    },
    resetButton: {
        width: '100%',
        backgroundColor: 'rgba(255,255,255,0.1)',
    },
    mapArea: {
        flex: 1,
        backgroundColor: 'rgba(0,0,0,0.3)',
        borderRadius: borderRadius.lg,
        overflow: 'hidden',
        borderWidth: 1,
        borderColor: 'rgba(255,255,255,0.1)',
        position: 'relative',
    },
    joystickContainer: {
        position: 'absolute',
        bottom: spacing.xl,
        right: spacing.xl,
    },

    floatingButton: {
        position: 'absolute',
        top: 16,
        right: 16,
        width: 48,
        height: 48,
        borderRadius: 24,
        backgroundColor: colors.primary.main,
        alignItems: 'center',
        justifyContent: 'center',
        ...Platform.select({
            ios: {
                shadowColor: '#000',
                shadowOffset: { width: 0, height: 2 },
                shadowOpacity: 0.25,
                shadowRadius: 8,
            },
            android: {
                elevation: 6,
            },
        }),
    },
    modalOverlay: {
        flex: 1,
        backgroundColor: 'rgba(0,0,0,0.7)',
        justifyContent: 'center',
        alignItems: 'center',
    },
    speedModal: {
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
        textAlign: 'center',
    },
    speedValue: {
        fontSize: fontSize['3xl'],
        fontWeight: fontWeight.bold,
        color: colors.text.white,
        textAlign: 'center',
        marginBottom: spacing.lg,
    },
    sliderContainer: {
        flexDirection: 'row',
        alignItems: 'center',
        width: '100%',
        gap: spacing.md,
        marginBottom: spacing.lg,
    },
    adjustBtn: {
        padding: spacing.sm,
        backgroundColor: 'rgba(255,255,255,0.1)',
        borderRadius: borderRadius.full,
    },
    sliderBar: {
        flex: 1,
        height: 6,
        backgroundColor: 'rgba(255,255,255,0.2)',
        borderRadius: 3,
        overflow: 'hidden',
    },
    sliderFill: {
        height: '100%',
        backgroundColor: colors.primary.main,
    },
    modalButtons: {
        flexDirection: 'row',
        gap: spacing.md,
    },
    modalButton: {
        flex: 1,
    },
    savedMapsContainer: {
        marginTop: spacing.lg,
        flex: 1,
    },
    mapsList: {
        flex: 1, // Fill remaining space
    },
    mapItem: {
        flexDirection: 'row',
        alignItems: 'center',
        justifyContent: 'space-between',
        padding: spacing.sm,
        backgroundColor: 'rgba(255,255,255,0.05)',
        borderRadius: borderRadius.md,
        marginBottom: spacing.xs,
    },
    mapName: {
        color: colors.text.white,
        fontSize: fontSize.sm,
        flex: 1,
        marginRight: spacing.sm,
    },
    mapActions: {
        flexDirection: 'row',
        alignItems: 'center',
        gap: spacing.sm,
    },
    iconButton: {
        padding: 4,
    },
    emptyStateText: {
        color: colors.text.muted,
        fontSize: fontSize.sm,
        fontStyle: 'italic',
        textAlign: 'center',
        marginTop: spacing.sm,
    },
    customModalOverlay: {
        position: 'absolute',
        top: 0,
        left: 0,
        right: 0,
        bottom: 0,
        backgroundColor: 'rgba(0,0,0,0.7)',
        justifyContent: 'center',
        alignItems: 'center',
        zIndex: 2000,
    },
    popupContainer: {
        backgroundColor: '#0A3F4C',
        borderRadius: borderRadius.lg,
        padding: spacing.xl,
        width: '80%',
        maxWidth: 400,
        borderWidth: 1,
        borderColor: 'rgba(255,255,255,0.1)',
    },
    popupHeader: {
        flexDirection: 'row',
        justifyContent: 'space-between',
        alignItems: 'center',
        marginBottom: spacing.lg,
    },
    deleteIconButton: {
        padding: spacing.sm,
        backgroundColor: 'rgba(255, 59, 48, 0.1)', // Light red background
        borderRadius: borderRadius.md,
    },
});

export default MappingScreen;

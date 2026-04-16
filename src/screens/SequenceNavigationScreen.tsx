import React, { useState, useEffect, useRef, useMemo } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert, ScrollView, Modal, TextInput, FlatList } from 'react-native';
import { useRobotStore } from '../store/robotStore';
import { useLanguageStore } from '../store/languageStore';
import { SafeAreaView } from 'react-native-safe-area-context';
import { Feather as Icon } from '@expo/vector-icons';
import MapDisplay, { MapMarker } from '../components/MapDisplay';
import { colors, spacing, fontSize, fontWeight, borderRadius } from '../theme';
import { Button } from '../components/ui';

interface SequencePoint {
    id: string;
    x: number;
    y: number;
    name: string;
    duration: number; // seconds
}

const SequenceNavigationScreen = () => {
    const { t } = useLanguageStore();
    const {
        api,
        robotPose,
        updateStatus,
        maxSpeed
    } = useRobotStore();

    // Map State
    const [mapImage, setMapImage] = useState<string | null>(null);
    const [mapResolution, setMapResolution] = useState(0.05);
    const [mapOrigin, setMapOrigin] = useState({ x: 0, y: 0 });
    const [mapHeight, setMapHeight] = useState(0);
    const [mapRotation, setMapRotation] = useState(0);

    // Sequence State
    const [points, setPoints] = useState<SequencePoint[]>([]);
    const [isAddingPoint, setIsAddingPoint] = useState(false);
    const [isRunning, setIsRunning] = useState(false);
    const [currentPointIndex, setCurrentPointIndex] = useState<number | null>(null);
    const [statusMessage, setStatusMessage] = useState('');

    // Modal State
    const [showPointModal, setShowPointModal] = useState(false);
    const [tempPoint, setTempPoint] = useState<{ x: number; y: number } | null>(null);
    const [pointName, setPointName] = useState('');
    const [pointDuration, setPointDuration] = useState('10');

    // Refs for sequence execution
    const isRunningRef = useRef(false);

    useEffect(() => {
        fetchMap(); // Initial fetch
        updateStatus(); // Initial status

        const interval = setInterval(() => {
            fetchMap();
            updateStatus();
        }, 1000); // Poll every 1s for better responsiveness

        return () => clearInterval(interval);
    }, []);

    const fetchMap = async () => {
        if (!api) return;
        try {
            const { data, error } = await api.getCurrentMap();
            if (!error && data && data.image_url) {
                setMapImage(data.image_url);
                if (data.resolution) setMapResolution(data.resolution);
                // Combine origin updates into single setState using functional update
                // to avoid stale closure issues in setInterval
                if (data.origin_x !== undefined || data.origin_y !== undefined) {
                    setMapOrigin(prev => ({
                        x: data.origin_x ?? prev.x,
                        y: data.origin_y ?? prev.y
                    }));
                }
                if (data.height) setMapHeight(data.height);
            }
        } catch (err) {
            console.error('Error fetching map:', err);
        }
    };

    const handleMapPress = (x: number, y: number) => {
        console.log('Map pressed at:', x, y);
        if (isAddingPoint) {
            setTempPoint({ x, y });
            setPointName(`Point ${points.length + 1}`);
            setPointDuration('10');
            setShowPointModal(true);
            setIsAddingPoint(false);
        }
    };

    const savePoint = () => {
        if (tempPoint) {
            const newPoint: SequencePoint = {
                id: Date.now().toString(),
                x: tempPoint.x,
                y: tempPoint.y,
                name: pointName,
                duration: parseInt(pointDuration) || 0,
            };
            setPoints([...points, newPoint]);
            setShowPointModal(false);
            setTempPoint(null);
        }
    };

    const removePoint = (id: string) => {
        setPoints(points.filter(p => p.id !== id));
    };

    const clearSequence = async () => {
        if (isRunning) return;
        setPoints([]);
        setStatusMessage('');

        // Ensure legacy navigation state is cleared
        if (api) {
            await updateStatus();
        }
    };

    const stopSequence = async () => {
        isRunningRef.current = false;
        setIsRunning(false);
        setCurrentPointIndex(null);
        setStatusMessage(t.sequence.sequenceStopped);
        if (api) {
            await api.cancelNavigation();
        }
    };

    const runSequence = async () => {
        if (points.length === 0 || !api) return;

        setIsRunning(true);
        isRunningRef.current = true;
        setStatusMessage(t.sequence.startSequence + '...');

        // Ensure robot is in navigation mode
        const { error: modeError } = await api.switchMappingMode(2);
        if (modeError) {
            setStatusMessage(`${t.common.error}: ${t.sequence.failedReach}`);
            setIsRunning(false);
            isRunningRef.current = false;
            return;
        }
        await new Promise(resolve => setTimeout(resolve, 1000));

        for (let i = 0; i < points.length; i++) {
            if (!isRunningRef.current) break;

            setCurrentPointIndex(i);
            const point = points[i];
            setStatusMessage(`${t.sequence.navigatingTo} ${point.name}...`);

            // Send navigation command
            const { error } = await api.sendLocationCoordinateNavigation(point.x, point.y, 0);
            if (error) {
                setStatusMessage(`${t.common.error}: ${error}`);
                await new Promise(resolve => setTimeout(resolve, 2000));
                continue;
            }

            await new Promise(resolve => setTimeout(resolve, 2000)); // Give robot time to start

            // Wait for navigation to complete using status API (like Python version)
            let navCompleted = false;
            while (isRunningRef.current && !navCompleted) {
                const { data: statusData, error: statusError } = await api.getNavigationStatus();
                if (!statusError && statusData) {
                    const navState = statusData.res ?? -1;
                    if (navState === 3) { // Navigation successful
                        setStatusMessage(`${t.sequence.arrivedAt} ${point.name}`);
                        navCompleted = true;
                    } else if (navState === 4) { // Navigation failed
                        setStatusMessage(`${t.sequence.failedReach} ${point.name}`);
                        break;
                    } else {
                        setStatusMessage(`${t.sequence.navigatingTo} ${point.name}... (${navState})`);
                    }
                }
                await new Promise(resolve => setTimeout(resolve, 1000));
            }

            if (!isRunningRef.current) break;

            // Wait for duration
            if (navCompleted && point.duration > 0) {
                setStatusMessage(`${t.sequence.waitingAt} ${point.name} (${point.duration}s)...`);
                for (let t_wait = 0; t_wait < point.duration; t_wait++) {
                    if (!isRunningRef.current) break;
                    setStatusMessage(`${t.sequence.waitingAt} ${point.name} (${point.duration - t_wait}s)...`);
                    await new Promise(resolve => setTimeout(resolve, 1000));
                }
            }
        }

        if (isRunningRef.current) {
            setStatusMessage(t.sequence.sequenceCompleted);
        }

        // Ensure navigation is cancelled/cleared at end
        // Removed cancelNavigation to prevent location reset
        // if (api) {
        //     await api.cancelNavigation();
        // }

        setIsRunning(false);
        isRunningRef.current = false;
        setCurrentPointIndex(null);
    };

    const toggleRotation = () => {
        setMapRotation(prev => (prev + 90) % 360);
    };

    // Memoize markers to prevent unnecessary re-renders
    const markers = useMemo<MapMarker[]>(() => {
        return points.map((p, index) => ({
            x: p.x,
            y: p.y,
            color: index === currentPointIndex ? '#F59E0B' : '#10B981',
            label: p.name
        }));
    }, [points, currentPointIndex]);

    return (
        <View style={styles.container}>
            {/* Header */}
            <View style={styles.header}>
                <View style={styles.headerTitleContainer}>
                    <Icon name="map-pin" size={24} color={colors.text.white} />
                    <View style={styles.divider} />
                    <Text style={styles.headerTitle}>{t.sequence.title}</Text>
                </View>
            </View>

            <View style={styles.content}>
                {/* Left Sidebar: Controls & List */}
                <View style={styles.sidebar}>
                    <View style={styles.controlsSection}>
                        <Button
                            variant={isAddingPoint ? "secondary" : "outline"}
                            onPress={() => setIsAddingPoint(!isAddingPoint)}
                            icon={<Icon name="plus-circle" size={16} color={isAddingPoint ? colors.text.white : colors.primary.main} />}
                            style={styles.controlButton}
                            textStyle={{ color: colors.text.white }}
                        >
                            {isAddingPoint ? t.sequence.tapMap : t.sequence.addPoint}
                        </Button>

                        {!isRunning ? (
                            <Button
                                variant="primary"
                                onPress={runSequence}
                                disabled={points.length === 0}
                                icon={<Icon name="play" size={16} color="white" />}
                                style={styles.controlButton}
                            >
                                {t.sequence.startSequence}
                            </Button>
                        ) : (
                            <Button
                                variant="primary"
                                onPress={stopSequence}
                                style={[styles.controlButton, { backgroundColor: colors.status.error }]}
                                icon={<Icon name="square" size={16} color="white" />}
                            >
                                {t.sequence.stop}
                            </Button>
                        )}

                        <Button
                            variant="secondary"
                            onPress={clearSequence}
                            disabled={isRunning || points.length === 0}
                            style={styles.controlButton}
                        >
                            {t.sequence.clearAll}
                        </Button>
                    </View>

                    <View style={styles.statusSection}>
                        <Text style={styles.statusText}>{statusMessage || t.sequence.ready}</Text>
                    </View>

                    <View style={styles.listSection}>
                        <Text style={styles.sectionTitle}>{t.sequence.sequencePoints} ({points.length})</Text>
                        <FlatList
                            data={points}
                            keyExtractor={item => item.id}
                            renderItem={({ item, index }) => (
                                <View style={[
                                    styles.pointItem,
                                    index === currentPointIndex && styles.activePointItem
                                ]}>
                                    <View style={styles.pointInfo}>
                                        <Text style={styles.pointName}>{index + 1}. {item.name}</Text>
                                        <Text style={styles.pointDetails}>Wait: {item.duration}s</Text>
                                    </View>
                                    {!isRunning && (
                                        <TouchableOpacity onPress={() => removePoint(item.id)} style={styles.deleteButton}>
                                            <Icon name="trash-2" size={16} color={colors.status.error} />
                                        </TouchableOpacity>
                                    )}
                                </View>
                            )}
                            ListEmptyComponent={
                                <Text style={styles.emptyText}>{t.sequence.noPoints}</Text>
                            }
                        />
                    </View>
                </View>

                {/* Right Area: Map */}
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
                        customMarkers={markers}
                        destination={currentPointIndex !== null ? points[currentPointIndex] : null}
                        onMapPress={handleMapPress}
                    />

                    <TouchableOpacity style={styles.floatingButton} onPress={toggleRotation}>
                        <Icon name="rotate-cw" color={colors.text.white} size={22} />
                    </TouchableOpacity>

                    {isAddingPoint && (
                        <View style={styles.addModeOverlay}>
                            <Text style={styles.addModeText}>{t.sequence.tapToPlace}</Text>
                        </View>
                    )}
                </View>
            </View>

            {/* Add Point Modal */}
            <Modal
                visible={showPointModal}
                transparent
                animationType="fade"
                onRequestClose={() => setShowPointModal(false)}>
                <View style={styles.modalOverlay}>
                    <View style={styles.modalContent}>
                        <Text style={styles.modalTitle}>{t.sequence.addSequencePoint}</Text>

                        <View style={styles.inputGroup}>
                            <Text style={styles.inputLabel}>{t.sequence.pointName}</Text>
                            <TextInput
                                style={styles.input}
                                value={pointName}
                                onChangeText={setPointName}
                                placeholder={t.sequence.pointNamePlaceholder}
                                placeholderTextColor={colors.text.muted}
                            />
                        </View>

                        <View style={styles.inputGroup}>
                            <Text style={styles.inputLabel}>{t.sequence.waitDuration}</Text>
                            <TextInput
                                style={styles.input}
                                value={pointDuration}
                                onChangeText={setPointDuration}
                                keyboardType="numeric"
                                placeholder="20"
                                placeholderTextColor={colors.text.muted}
                            />
                        </View>

                        <View style={styles.modalButtons}>
                            <Button variant="outline" onPress={() => setShowPointModal(false)} style={styles.modalButton}>
                                {t.common.cancel}
                            </Button>
                            <Button variant="primary" onPress={savePoint} style={styles.modalButton}>
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
        paddingHorizontal: spacing.lg,
        paddingVertical: spacing.md,
        borderBottomWidth: 1,
        borderBottomColor: 'rgba(255,255,255,0.1)',
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
        fontSize: fontSize.lg,
        color: colors.text.white,
        fontWeight: fontWeight.medium,
    },
    content: {
        flex: 1,
        flexDirection: 'row',
        padding: spacing.lg,
        gap: spacing.lg,
    },
    sidebar: {
        width: 300,
        backgroundColor: 'rgba(0,0,0,0.2)',
        borderRadius: borderRadius.lg,
        padding: spacing.md,
        gap: spacing.lg,
    },
    controlsSection: {
        gap: spacing.sm,
    },
    controlButton: {
        width: '100%',
    },
    statusSection: {
        padding: spacing.sm,
        backgroundColor: 'rgba(0,0,0,0.3)',
        borderRadius: borderRadius.sm,
        minHeight: 40,
        justifyContent: 'center',
    },
    statusText: {
        color: colors.text.white,
        fontSize: fontSize.sm,
        textAlign: 'center',
    },
    listSection: {
        flex: 1,
    },
    sectionTitle: {
        color: colors.text.white,
        fontSize: fontSize.md,
        fontWeight: fontWeight.bold,
        marginBottom: spacing.sm,
    },
    pointItem: {
        flexDirection: 'row',
        alignItems: 'center',
        justifyContent: 'space-between',
        backgroundColor: 'rgba(255,255,255,0.05)',
        padding: spacing.sm,
        borderRadius: borderRadius.sm,
        marginBottom: spacing.xs,
    },
    activePointItem: {
        backgroundColor: 'rgba(245, 158, 11, 0.2)', // Orange tint
        borderColor: '#F59E0B',
        borderWidth: 1,
    },
    pointInfo: {
        flex: 1,
    },
    pointName: {
        color: colors.text.white,
        fontSize: fontSize.sm,
        fontWeight: fontWeight.bold,
    },
    pointDetails: {
        color: colors.text.muted,
        fontSize: fontSize.xs,
    },
    deleteButton: {
        padding: spacing.xs,
    },
    emptyText: {
        color: colors.text.muted,
        fontSize: fontSize.sm,
        textAlign: 'center',
        marginTop: spacing.lg,
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
        elevation: 5,
    },
    addModeOverlay: {
        position: 'absolute',
        top: 16,
        left: '50%',
        transform: [{ translateX: -100 }],
        backgroundColor: colors.primary.main,
        paddingHorizontal: spacing.lg,
        paddingVertical: spacing.sm,
        borderRadius: borderRadius.full,
        elevation: 5,
    },
    addModeText: {
        color: colors.text.white,
        fontWeight: fontWeight.bold,
    },
    modalOverlay: {
        flex: 1,
        backgroundColor: 'rgba(0,0,0,0.7)',
        justifyContent: 'center',
        alignItems: 'center',
    },
    modalContent: {
        backgroundColor: '#0A3F4C',
        borderRadius: borderRadius.lg,
        padding: spacing.xl,
        width: '80%',
        maxWidth: 400,
        gap: spacing.lg,
    },
    modalTitle: {
        color: colors.text.white,
        fontSize: fontSize.xl,
        fontWeight: fontWeight.bold,
        textAlign: 'center',
    },
    inputGroup: {
        gap: spacing.xs,
    },
    inputLabel: {
        color: colors.text.white,
        fontSize: fontSize.sm,
    },
    input: {
        backgroundColor: 'rgba(255,255,255,0.1)',
        borderRadius: borderRadius.md,
        padding: spacing.md,
        color: colors.text.white,
        fontSize: fontSize.md,
    },
    modalButtons: {
        flexDirection: 'row',
        gap: spacing.md,
    },
    modalButton: {
        flex: 1,
    },
});

export default SequenceNavigationScreen;

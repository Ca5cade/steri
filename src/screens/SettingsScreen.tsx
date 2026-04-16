import React, { useState, useEffect } from 'react';
import { View, Text, StyleSheet, TouchableOpacity, Alert, Modal, TextInput, ScrollView } from 'react-native';
import { useNavigation } from '@react-navigation/native';
import { Feather as Icon } from '@expo/vector-icons';
import { colors, spacing, fontSize, fontWeight, borderRadius } from '../theme';
import { SafeAreaView } from 'react-native-safe-area-context';
import { useLanguageStore } from '../store/languageStore';
import Slider from '@react-native-community/slider';
import { auth } from '../config/firebase';
import { updatePassword } from 'firebase/auth';
import { SoundService } from '../services/SoundService';

import { LanguageSwitcher } from '../components/LanguageSwitcher';
import { Button } from '../components/ui';

const SettingsScreen = () => {
    const navigation = useNavigation();
    const { t } = useLanguageStore();

    // Audio State
    const [volume, setVolume] = useState(50);

    // Password Update State
    const [showPasswordModal, setShowPasswordModal] = useState(false);
    const [newPassword, setNewPassword] = useState('');
    const [confirmPassword, setConfirmPassword] = useState('');
    const [loading, setLoading] = useState(false);

    useEffect(() => {
        loadVolume();
    }, []);

    const loadVolume = async () => {
        const data = await SoundService.getVolume();
        if (data && typeof data.volume === 'number') {
            // API returns 0-1, Slider expects 0-100
            setVolume(data.volume * 100);
        }
    };

    const handleVolumeChange = (value: number) => {
        setVolume(value);
        // Debounce can be added if needed, but slider usually handles drags well
    };

    const handleVolumeComplete = async (value: number) => {
        await SoundService.setVolume(Math.round(value));
    };

    const handleChangePassword = async () => {
        if (!newPassword || !confirmPassword) {
            Alert.alert(t.common.error, t.settings.fillAllFields);
            return;
        }

        if (newPassword !== confirmPassword) {
            Alert.alert(t.common.error, t.settings.passwordMismatch);
            return;
        }

        if (auth.currentUser) {
            setLoading(true);
            try {
                await updatePassword(auth.currentUser, newPassword);
                Alert.alert(t.common.success, t.settings.passwordUpdated);
                setShowPasswordModal(false);
                setNewPassword('');
                setConfirmPassword('');
            } catch (error: any) {
                Alert.alert(t.common.error, t.settings.passwordUpdateError + ": " + error.message);
            } finally {
                setLoading(false);
            }
        }
    };

    const handleLogout = () => {
        Alert.alert(
            t.common.logout,
            t.settings.logoutConfirm,
            [
                {
                    text: t.common.cancel,
                    style: "cancel"
                },
                {
                    text: t.common.logout,
                    style: "destructive",
                    onPress: () => {
                        // Reset navigation to Login screen
                        navigation.reset({
                            index: 0,
                            routes: [{ name: 'Login' }],
                        });
                    }
                }
            ]
        );
    };

    const userEmail = auth.currentUser?.email || "user@example.com";

    return (
        <SafeAreaView style={styles.container} edges={['top']}>
            <View style={styles.header}>
                <Text style={styles.headerTitle}>{t.settings.title}</Text>
                <LanguageSwitcher />
            </View>

            <ScrollView style={styles.content}>

                {/* User Settings Section */}
                <View style={styles.section}>
                    <Text style={styles.sectionTitle}>{t.settings.userSettings}</Text>

                    <View style={styles.infoRow}>
                        <Text style={styles.label}>{t.settings.email}</Text>
                        <Text style={styles.value}>{userEmail}</Text>
                    </View>

                    <Button
                        variant="outline"
                        onPress={() => setShowPasswordModal(true)}
                        style={styles.changePassBtn}
                        icon={<Icon name="lock" size={16} color={colors.primary.main} />}
                    >
                        {t.settings.changePassword}
                    </Button>
                </View>

                {/* Audio Settings Section */}
                <View style={styles.section}>
                    <Text style={styles.sectionTitle}>{t.settings.audioSettings}</Text>

                    <View style={styles.sliderRow}>
                        <Text style={styles.label}>{t.settings.volume}: {Math.round(volume)}%</Text>
                        <View style={styles.sliderContainer}>
                            <Icon name="volume-1" size={20} color={colors.text.muted} />
                            <Slider
                                style={styles.slider}
                                minimumValue={0}
                                maximumValue={100}
                                step={1}
                                value={volume}
                                onValueChange={handleVolumeChange}
                                onSlidingComplete={handleVolumeComplete}
                                minimumTrackTintColor={colors.primary.main}
                                maximumTrackTintColor="rgba(255,255,255,0.2)"
                                thumbTintColor={colors.primary.light}
                            />
                            <Icon name="volume-2" size={20} color={colors.text.white} />
                        </View>
                    </View>
                </View>

                <TouchableOpacity style={styles.logoutButton} onPress={handleLogout}>
                    <Icon name="log-out" size={24} color={colors.text.white} />
                    <Text style={styles.logoutText}>{t.common.logout}</Text>
                </TouchableOpacity>

            </ScrollView>

            {/* Password Change Modal */}
            <Modal
                visible={showPasswordModal}
                transparent
                animationType="fade"
                onRequestClose={() => setShowPasswordModal(false)}
            >
                <View style={styles.modalOverlay}>
                    <View style={styles.modalContent}>
                        <Text style={styles.modalTitle}>{t.settings.changePassword}</Text>

                        <View style={styles.inputGroup}>
                            <Text style={styles.inputLabel}>{t.settings.newPassword}</Text>
                            <TextInput
                                style={styles.input}
                                value={newPassword}
                                onChangeText={setNewPassword}
                                secureTextEntry
                                placeholderTextColor={colors.text.muted}
                            />
                        </View>

                        <View style={styles.inputGroup}>
                            <Text style={styles.inputLabel}>{t.settings.confirmNewPassword}</Text>
                            <TextInput
                                style={styles.input}
                                value={confirmPassword}
                                onChangeText={setConfirmPassword}
                                secureTextEntry
                                placeholderTextColor={colors.text.muted}
                            />
                        </View>

                        <View style={styles.modalButtons}>
                            <Button variant="outline" onPress={() => setShowPasswordModal(false)} style={styles.modalButton}>
                                {t.common.cancel}
                            </Button>
                            <Button
                                variant="primary"
                                onPress={handleChangePassword}
                                loading={loading}
                                style={styles.modalButton}
                            >
                                {t.common.save}
                            </Button>
                        </View>
                    </View>
                </View>
            </Modal>
        </SafeAreaView>
    );
};

const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: '#0A3F4C',
    },
    header: {
        padding: spacing.lg,
        borderBottomWidth: 1,
        borderBottomColor: 'rgba(255,255,255,0.1)',
        flexDirection: 'row',
        justifyContent: 'space-between',
        alignItems: 'center',
    },
    headerTitle: {
        fontSize: fontSize.xl,
        fontWeight: fontWeight.bold,
        color: colors.text.white,
    },
    content: {
        flex: 1,
        padding: spacing.lg,
    },
    section: {
        marginBottom: spacing.xl,
        backgroundColor: 'rgba(255,255,255,0.05)',
        padding: spacing.lg,
        borderRadius: borderRadius.lg,
    },
    sectionTitle: {
        fontSize: fontSize.lg,
        fontWeight: fontWeight.bold,
        color: colors.text.white,
        marginBottom: spacing.lg,
        borderBottomWidth: 1,
        borderBottomColor: 'rgba(255,255,255,0.1)',
        paddingBottom: spacing.sm,
    },
    infoRow: {
        marginBottom: spacing.md,
    },
    label: {
        color: colors.text.muted,
        fontSize: fontSize.sm,
        marginBottom: 4,
    },
    value: {
        color: colors.text.white,
        fontSize: fontSize.md,
        fontWeight: fontWeight.medium,
    },
    changePassBtn: {
        marginTop: spacing.sm,
        borderColor: colors.primary.main,
    },
    sliderRow: {
        gap: spacing.sm,
    },
    sliderContainer: {
        flexDirection: 'row',
        alignItems: 'center',
        gap: spacing.md,
    },
    slider: {
        flex: 1,
        height: 40,
    },
    logoutButton: {
        flexDirection: 'row',
        alignItems: 'center',
        justifyContent: 'center',
        gap: spacing.md,
        backgroundColor: colors.status.error,
        padding: spacing.md,
        borderRadius: borderRadius.md,
        marginTop: spacing.xl,
    },
    logoutText: {
        color: colors.text.white,
        fontSize: fontSize.md,
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
        width: '85%',
        maxWidth: 400,
        gap: spacing.lg,
    },
    modalTitle: {
        color: colors.text.white,
        fontSize: fontSize.lg,
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
        marginTop: spacing.md,
    },
    modalButton: {
        flex: 1,
    }
});

export default SettingsScreen;

import React, { useState } from 'react';
import { View, Text, TouchableOpacity, StyleSheet, Modal } from 'react-native';
import { useLanguageStore } from '../store/languageStore';
import { Language } from '../constants/translations';
import { spacing, borderRadius, colors, fontSize, fontWeight } from '../theme';

const flags: Record<Language, string> = {
    en: '🇬🇧',
    fr: '🇫🇷',
    ar: '🇹🇳',
};

const languageNames: Record<Language, string> = {
    en: 'English',
    fr: 'Français',
    ar: 'Arabic (TN)',
};

export const LanguageSwitcher: React.FC = () => {
    const { currentLanguage, setLanguage } = useLanguageStore();
    const [modalVisible, setModalVisible] = useState(false);

    const handleSelectLanguage = (lang: Language) => {
        setLanguage(lang);
        setModalVisible(false);
    };

    return (
        <>
            <TouchableOpacity
                style={styles.button}
                onPress={() => setModalVisible(true)}
            >
                <Text style={styles.flag}>{flags[currentLanguage]}</Text>
            </TouchableOpacity>

            <Modal
                visible={modalVisible}
                transparent
                animationType="fade"
                onRequestClose={() => setModalVisible(false)}
            >
                <TouchableOpacity
                    style={styles.modalOverlay}
                    activeOpacity={1}
                    onPress={() => setModalVisible(false)}
                >
                    <View style={styles.modalContent}>
                        <Text style={styles.modalTitle}>Select Language</Text>
                        {(Object.keys(flags) as Language[]).map((lang) => (
                            <TouchableOpacity
                                key={lang}
                                style={[
                                    styles.option,
                                    currentLanguage === lang && styles.selectedOption
                                ]}
                                onPress={() => handleSelectLanguage(lang)}
                            >
                                <Text style={styles.optionFlag}>{flags[lang]}</Text>
                                <Text style={[
                                    styles.optionText,
                                    currentLanguage === lang && styles.selectedOptionText
                                ]}>
                                    {languageNames[lang]}
                                </Text>
                            </TouchableOpacity>
                        ))}
                    </View>
                </TouchableOpacity>
            </Modal>
        </>
    );
};

const styles = StyleSheet.create({
    button: {
        padding: spacing.sm,
        backgroundColor: 'rgba(255, 255, 255, 0.1)',
        borderRadius: borderRadius.md,
    },
    flag: {
        fontSize: 24,
    },
    modalOverlay: {
        flex: 1,
        backgroundColor: 'rgba(0, 0, 0, 0.5)',
        justifyContent: 'center',
        alignItems: 'center',
    },
    modalContent: {
        backgroundColor: colors.slate[800],
        borderRadius: borderRadius.lg,
        padding: spacing.lg,
        width: 280,
        // Fallback checks for theme usage compatibility
        borderWidth: 1,
        borderColor: 'rgba(255,255,255,0.1)',
    },
    modalTitle: {
        fontSize: fontSize.lg,
        fontWeight: fontWeight.bold,
        color: colors.text.white,
        marginBottom: spacing.md,
        textAlign: 'center',
    },
    option: {
        flexDirection: 'row',
        alignItems: 'center',
        paddingVertical: spacing.md,
        paddingHorizontal: spacing.md,
        borderRadius: borderRadius.md,
        marginBottom: spacing.xs,
    },
    selectedOption: {
        backgroundColor: colors.primary.main || '#00A3FF',
    },
    optionFlag: {
        fontSize: 24,
        marginRight: spacing.md,
    },
    optionText: {
        fontSize: fontSize.md,
        color: colors.text.white,
    },
    selectedOptionText: {
        fontWeight: fontWeight.bold,
    },
});

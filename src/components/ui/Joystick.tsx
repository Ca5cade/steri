import React from 'react';
import { StyleSheet, View, ViewStyle } from 'react-native';
import { GestureDetector, Gesture } from 'react-native-gesture-handler';
import Animated, {
    useSharedValue,
    useAnimatedStyle,
    withSpring,
    runOnJS,
} from 'react-native-reanimated';
import { colors } from '../../theme';

interface JoystickProps {
    onMove?: (data: { x: number; y: number; angle: number; intensity: number }) => void;
    onStop?: () => void;
    size?: number;
    color?: string;
    outerColor?: string;
    style?: ViewStyle;
}

export const Joystick: React.FC<JoystickProps> = ({
    onMove,
    onStop,
    size = 150,
    color = colors.primary.main,
    outerColor = 'rgba(255,255,255,0.1)',
    style,
}) => {
    const translateX = useSharedValue(0);
    const translateY = useSharedValue(0);
    const knobSize = size / 2.5;
    const maxDistance = (size - knobSize) / 2;

    const gesture = Gesture.Pan()
        .onUpdate((event) => {
            const x = event.translationX;
            const y = event.translationY;

            const distance = Math.sqrt(x * x + y * y);
            const angle = Math.atan2(y, x);

            let newX = x;
            let newY = y;

            if (distance > maxDistance) {
                newX = Math.cos(angle) * maxDistance;
                newY = Math.sin(angle) * maxDistance;
            }

            translateX.value = newX;
            translateY.value = newY;

            if (onMove) {
                const intensity = Math.min(distance / maxDistance, 1);
                // Normalize x and y to -1 to 1
                const normalizedX = newX / maxDistance;
                const normalizedY = newY / maxDistance;
                runOnJS(onMove)({ x: normalizedX, y: normalizedY, angle, intensity });
            }
        })
        .onEnd(() => {
            translateX.value = withSpring(0);
            translateY.value = withSpring(0);
            if (onStop) {
                runOnJS(onStop)();
            }
        });

    const knobStyle = useAnimatedStyle(() => {
        return {
            transform: [
                { translateX: translateX.value },
                { translateY: translateY.value },
            ],
        };
    });

    return (
        <View style={[styles.container, { width: size, height: size, borderRadius: size / 2 }, style]}>
            <View style={[styles.base, { width: size, height: size, borderRadius: size / 2, backgroundColor: outerColor }]} />
            <GestureDetector gesture={gesture}>
                <Animated.View
                    style={[
                        styles.knob,
                        {
                            width: knobSize,
                            height: knobSize,
                            borderRadius: knobSize / 2,
                            backgroundColor: color,
                        },
                        knobStyle,
                    ]}
                />
            </GestureDetector>
        </View>
    );
};

const styles = StyleSheet.create({
    container: {
        alignItems: 'center',
        justifyContent: 'center',
    },
    base: {
        position: 'absolute',
        borderWidth: 2,
        borderColor: 'rgba(255,255,255,0.2)',
    },
    knob: {
        elevation: 5,
        shadowColor: '#000',
        shadowOffset: { width: 0, height: 2 },
        shadowOpacity: 0.3,
        shadowRadius: 3,
    },
});

export default Joystick;

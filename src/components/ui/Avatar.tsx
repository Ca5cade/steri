import React from 'react';
import {View, Image, Text, StyleSheet, ImageStyle, StyleProp} from 'react-native';
import {colors, borderRadius, fontSize, fontWeight} from '../../theme';

interface AvatarProps {
  source?: string;
  name?: string;
  size?: 'sm' | 'md' | 'lg' | 'xl';
  style?: StyleProp<ImageStyle>;
}

export const Avatar: React.FC<AvatarProps> = ({
  source,
  name = '',
  size = 'md',
  style,
}) => {
  const getInitials = (fullName: string): string => {
    const names = fullName.trim().split(' ');
    if (names.length >= 2) {
      return (names[0][0] + names[names.length - 1][0]).toUpperCase();
    }
    return fullName.slice(0, 2).toUpperCase();
  };

  const sizeStyles = {
    sm: {container: 32, text: fontSize.sm},
    md: {container: 48, text: fontSize.base},
    lg: {container: 64, text: fontSize.lg},
    xl: {container: 96, text: fontSize['2xl']},
  };

  const currentSize = sizeStyles[size];

  if (source) {
    return (
      <Image
        source={{uri: source}}
        style={[
          styles.image,
          {
            width: currentSize.container,
            height: currentSize.container,
            borderRadius: currentSize.container / 2,
          },
          style,
        ]}
      />
    );
  }

  return (
    <View
      style={[
        styles.fallback,
        {
          width: currentSize.container,
          height: currentSize.container,
          borderRadius: currentSize.container / 2,
        },
        style,
      ]}>
      <Text style={[styles.initials, {fontSize: currentSize.text}]}>
        {getInitials(name)}
      </Text>
    </View>
  );
};

const styles = StyleSheet.create({
  image: {
    backgroundColor: colors.primary.main,
  },
  fallback: {
    backgroundColor: colors.primary.main,
    alignItems: 'center',
    justifyContent: 'center',
  },
  initials: {
    color: colors.text.white,
    fontWeight: fontWeight.bold,
  },
});

export default Avatar;


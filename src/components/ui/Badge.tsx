import React from 'react';
import {View, Text, StyleSheet, ViewStyle} from 'react-native';
import {colors, borderRadius, spacing, fontSize, fontWeight} from '../../theme';

type BadgeVariant =
  | 'default'
  | 'success'
  | 'warning'
  | 'error'
  | 'info'
  | 'secondary'
  | 'green'
  | 'blue'
  | 'orange'
  | 'purple'
  | 'red'
  | 'pink'
  | 'yellow';

interface BadgeProps {
  children: React.ReactNode;
  variant?: BadgeVariant;
  style?: ViewStyle;
  icon?: React.ReactNode;
}

export const Badge: React.FC<BadgeProps> = ({
  children,
  variant = 'default',
  style,
  icon,
}) => {
  const getColors = () => {
    switch (variant) {
      case 'success':
      case 'green':
        return colors.badge.green;
      case 'warning':
      case 'orange':
        return colors.badge.orange;
      case 'error':
      case 'red':
        return colors.badge.red;
      case 'info':
      case 'blue':
        return colors.badge.blue;
      case 'purple':
        return colors.badge.purple;
      case 'pink':
        return colors.badge.pink;
      case 'yellow':
        return colors.badge.yellow;
      case 'secondary':
        return {bg: colors.primary.main, text: colors.text.white};
      default:
        return {bg: '#E5E7EB', text: '#374151'};
    }
  };

  const badgeColors = getColors();

  return (
    <View
      style={[
        styles.badge,
        {backgroundColor: badgeColors.bg},
        style,
      ]}>
      {icon && <View style={styles.icon}>{icon}</View>}
      <Text style={[styles.text, {color: badgeColors.text}]}>{children}</Text>
    </View>
  );
};

const styles = StyleSheet.create({
  badge: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: spacing.sm,
    paddingVertical: spacing.xs,
    borderRadius: borderRadius.full,
    alignSelf: 'flex-start',
  },
  text: {
    fontSize: fontSize.xs,
    fontWeight: fontWeight.medium,
  },
  icon: {
    marginRight: spacing.xs,
  },
});

export default Badge;

